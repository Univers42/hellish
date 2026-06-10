/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers1.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:31:19 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 17:16:01 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "heredoc_private.h"

/* Try to defer this heredoc's materialisation to execution time: from a stored
   body (functions / pre-extracted hd_src) or, for a same-line -c/script
   heredoc, straight from the unread remainder of rl.buff. Returns true if the
   body was captured onto the node (caller skips eager temp-file writing). */
static bool	try_defer_heredoc(t_shell *state, t_ast_node *curr,
				bool is_pipeline)
{
	if ((state->gather_in_func || (!is_pipeline && state->hd_src))
		&& capture_heredoc_to_node(state, curr))
		return (true);
	if (!is_pipeline && !state->gather_in_func && !state->hd_src
		&& capture_heredoc_from_buff(state, curr))
		return (true);
	return (false);
}

/* Attempt to defer a heredoc (capture body to node) or, failing that,
   materialize it eagerly (write_heredoc expands the body and attaches a
   pipe or temp-file backing to the redirect slot).  The sep.ctx guard
   handles the edge case where the delimiter was a pure variable ($X)
   that expanded to empty: we fall back to a literal '$' so write_heredoc
   does not loop forever looking for an empty delimiter line.  Returns -1
   if deferred or on error; the redir_idx from ft_mktemp otherwise. */
static int	create_heredoc_tempfile(t_shell *state, t_ast_node *curr,
				bool is_pipeline)
{
	int				wr;
	t_string		sep;
	t_hdoc			req;

	if (try_defer_heredoc(state, curr, is_pipeline))
		return (-1);
	wr = ft_mktemp(state, curr);
	if (wr < 0)
		return (-1);
	sep = word_to_hrdoc_string(((t_ast_node *)curr->children.ctx)[1]);
	if (!sep.ctx)
	{
		vec_init(&sep);
		sep.elem_size = 1;
		vec_push_char(&sep, '$');
	}
	if (!vec_ensure_space_n(&sep, 1))
		return (xfree(sep.ctx), -1);
	((char *)sep.ctx)[sep.len] = '\0';
	req = create_heredoc((char *)sep.ctx,
			!contains_quotes(((t_ast_node *)curr->children.ctx)[1]),
			ft_strncmp(((t_ast_node *)curr->children.ctx)[0].token.start,
				"<<-", 3) == 0, is_pipeline);
	return (write_heredoc(state, wr, &req), xfree(sep.ctx), curr->redir_idx);
}

/* Process all AST_REDIRECT children of `parent` in the range [start,end).
   For each heredoc (TT_HEREDOC token) we call create_heredoc_tempfile;
   for all other redirect types we call gather_heredoc which handles non-
   heredoc redirects as a no-op and heredocs the same way.  The is_pipeline
   flag propagates the context so pipeline heredocs can be deferred. */
void	process_redirect_group(t_shell *state, t_ast_node *parent,
								size_t start, size_t end)
{
	size_t		k;
	bool		is_pipeline;
	t_ast_node	*curr;
	t_token		tt;

	is_pipeline = parent->node_type == AST_COMMAND_PIPELINE;
	k = start - 1;
	while (++k < end)
	{
		curr = &((t_ast_node *)parent->children.ctx)[k];
		if (curr->node_type != AST_REDIRECT)
			continue ;
		tt = ((t_ast_node *)curr->children.ctx)[0].token;
		if (tt.tt == TT_HEREDOC)
			create_heredoc_tempfile(state, curr, is_pipeline);
		else
			gather_heredoc(state, curr, is_pipeline);
	}
}

/* Return true for node types that carry no heredoc operators and need no
   recursion: NULL nodes, process substitutions (they have their own gather
   pass), raw AST_TOKEN nodes, and AST_WORD nodes (leaf words). */
bool	should_skip_node(t_ast_node *node)
{
	if (!node)
		return (true);
	if (node->node_type == AST_PROC_SUB)
		return (true);
	if (node->node_type == AST_TOKEN)
		return (true);
	if (node->node_type == AST_WORD)
		return (true);
	return (false);
}

/* Recurse into a child that is NOT an AST_REDIRECT, advancing *idx.  The
   in_pipeline flag is inferred from the parent's type so pipeline heredocs
   are flagged correctly even when we do not have a dedicated is_pipe
   parameter here. */
void	recurse_non_redirect_child(t_shell *state,
									t_ast_node *node,
									size_t *idx)
{
	t_ast_node	*child;

	if (!node->children.ctx || *idx >= node->children.len)
	{
		(*idx)++;
		return ;
	}
	child = &((t_ast_node *)node->children.ctx)[*idx];
	if (!should_skip_node(child))
		gather_heredocs(state, child,
			(node->node_type == AST_COMMAND_PIPELINE));
	(*idx)++;
}
