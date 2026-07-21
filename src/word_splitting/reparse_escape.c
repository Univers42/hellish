/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparser3.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:46 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:46 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"
#include "parena.h"

/* Stamp every child node with a pointer to the full-word token covering the
   whole original raw text. With the arena gate open one copy is SHARED by
   all children: parena_free is a no-op on arena blocks, so the "free it
   once per child" hazard that forced per-child copies cannot fire. With
   the gate closed (eval/source) each child still gets its own heap copy,
   because there free_node really frees each pointer independently. */
static void	set_full_word_for_children(void *ctx, size_t len,
				t_token_old full_word)
{
	size_t		i;
	t_token_old	*p;
	t_token_old	*shared;

	shared = NULL;
	if (parena()->on)
	{
		shared = parena_alloc(sizeof(t_token_old));
		if (shared)
			*shared = full_word;
	}
	i = 0;
	while (i < len)
	{
		p = shared;
		if (!p)
		{
			p = parena_alloc(sizeof(t_token_old));
			if (p)
				*p = full_word;
		}
		((t_ast_node *)ctx)[i++].token.full_word = p;
	}
}

/* The characters that give the reparser something to do — quoting,
   expansion, assignment, glob, tilde, brace, history bang — as a lookup
   table ('  "  \  $  `  =  {  }  ~  *  ?  [  ]  !). The old ft_strchr
   walked a 14-byte needle per CHARACTER of every word; this is one load. */
static const unsigned char	g_rp_spec[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/* True for a raw TT_WORD containing none of the reparse-special set.
   Such a word's reparse output would be a single TT_WORD subtoken over
   the same slice with split_eligible false (only command-substitution
   output is IFS-split) — identical to the raw child the parser already
   built, so the fast path keeps it untouched. full_word stays NULL
   (every consumer NULL-checks it). Roughly two thirds of real-script
   words take this path. */
static bool	word_is_plain(const t_token *tok)
{
	int	i;

	if (tok->tt != TT_WORD)
		return (false);
	i = 0;
	while (i < tok->len)
	{
		if (g_rp_spec[(unsigned char)tok->start[i]])
			return (false);
		i++;
	}
	return (true);
}

/* Recursively walk every child and call reparse_words on it. We skip
   AST_PROC_SUB because process substitution bodies have already been parsed
   as a sub-shell; re-parsing their word tokens would corrupt the AST since
   the tokens are borrowed from an inner parse context, not this one. */
static void	reparse_children_words(t_ast_node *node)
{
	size_t		i;
	t_ast_node	*child;

	i = 0;
	while (i < node->children.len)
	{
		child = &((t_ast_node *)node->children.ctx)[i];
		if (child->node_type != AST_PROC_SUB)
			reparse_words(child);
		i++;
	}
}

/* The second-pass entry point for a whole AST subtree. For AST_WORD nodes
   with exactly one child (the raw token), replace the child vec with the
   fully parsed subtoken tree from reparse_word(). The temp/new_ctx dance
   avoids a double-free when reparse_word returns the same backing allocation
   (it may reuse the child vec if it only adds one node). With the arena
   gate open the outgrown raw child is simply DROPPED (its tokens borrow
   the lexer buffer and its children array is arena — the walk would be a
   pure no-op, and it ran 161k times on a 50k-line parse). The full_word
   pointer is stamped on every new child so the expander can reconstruct
   the original text for error messages and ${!var} indirect references. */
void	reparse_words(t_ast_node *node)
{
	t_ast_node	temp;
	t_token_old	full_word;
	t_token		tok;
	void		*new_ctx;
	size_t		new_len;

	if (node->node_type == AST_WORD)
	{
		ft_assert(node->children.len == 1);
		tok = ((t_ast_node *)node->children.ctx)[0].token;
		if (word_is_plain(&tok))
			return ;
		full_word = create_token_old(tok.start, tok.len, true);
		temp = *node;
		*node = reparse_word(tok);
		new_ctx = node->children.ctx;
		new_len = node->children.len;
		if (temp.children.ctx != new_ctx && !parena()->on)
			free_ast(&temp);
		set_full_word_for_children(new_ctx, new_len, full_word);
	}
	reparse_children_words(node);
}
