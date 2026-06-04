/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_word.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:41:16 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:41:16 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "brace_expand.h"
#include "decomposer.h"
#include "sys.h"

char	*arith_expand(t_shell *state, const char *expr, int len);

/* A word participates in brace expansion when an *unquoted* part (a TT_WORD
   subtoken) contains a '{': bash brace-expands the unquoted `/{a,b}` in
   "$LFS"/{a,b} while quoted/var subtokens are carried along as literal text.
   Braces that live only inside quotes ("{a,b}") have no TT_WORD subtoken and
   so are correctly left untouched. */
static bool	word_has_unquoted_brace(t_ast_node *node)
{
	size_t		i;
	int			j;
	t_ast_node	*c;

	if (!node->children.ctx || node->children.len == 0)
		return (false);
	i = 0;
	while (i < node->children.len)
	{
		c = &((t_ast_node *)node->children.ctx)[i++];
		if (c->node_type != AST_TOKEN || c->token.tt != TT_WORD)
			continue ;
		j = 0;
		while (j < c->token.len)
			if (c->token.start[j++] == '{')
				return (true);
	}
	return (false);
}

static void	expand_brace_result(t_shell *state, t_vec *results, t_vec *args)
{
	char		*fs;
	t_ast_node	w;
	t_token		tmp;
	size_t		i;

	i = 0;
	while (i < results->len)
	{
		fs = ((char **)results->ctx)[i++];
		tmp.start = fs;
		tmp.len = (int)ft_strlen(fs);
		tmp.tt = TT_WORD;
		w = reparse_word(tmp);
		expand_word(state, &w, args, false);
		free(fs);
	}
}

/* Brace-expand `node` into multiple words, re-lexing each result and running
   it through the full expansion pipeline. Returns true if it handled `node`. */
bool	try_brace_expand(t_shell *state, t_ast_node *node, t_vec *args)
{
	t_string	flat;
	char		*fs;
	t_vec		results;

	if (!word_has_unquoted_brace(node))
		return (false);
	flat = word_to_brace_src(*node);
	fs = ft_strndup((char *)flat.ctx, flat.len);
	free(flat.ctx);
	if (brace_find_expandable(fs, &(int){0}) < 0)
		return (free(fs), false);
	results = brace_expand_str(fs);
	free(fs);
	expand_brace_result(state, &results, args);
	return (free(results.ctx), free_ast(node), true);
}

static bool	has_plain_literal_meta(char c, int *lbr, int i)
{
	if (c == '$' || c == '`' || c == '*' || c == '?' || c == '~'
		|| c == '{' || c == '\\' || c == '\'' || c == '"'
		|| c == ' ' || c == '\t' || c == '\n')
		return (true);
	if (c == '[')
		*lbr = i;
	else if (c == ']' && *lbr >= 0 && i > *lbr + 1)
		return (true);
	return (false);
}

/* A single literal TT_WORD subtoken with no metacharacter, quote or blank
   expands to itself as exactly one field. */
bool	word_is_plain_literal(t_ast_node *node)
{
	t_token	*t;
	int		i;
	int		lbr;
	char	c;

	if (node->children.len != 1
		|| ((t_ast_node *)node->children.ctx)[0].node_type != AST_TOKEN)
		return (false);
	t = &((t_ast_node *)node->children.ctx)[0].token;
	if (t->tt != TT_WORD)
		return (false);
	i = 0;
	lbr = -1;
	while (i < t->len)
	{
		c = t->start[i];
		if (has_plain_literal_meta(c, &lbr, i))
			return (false);
		i++;
	}
	return (true);
}
