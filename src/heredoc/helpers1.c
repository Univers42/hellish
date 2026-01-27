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

static int	create_heredoc_tempfile(t_shell *state, t_ast_node *curr,
				bool is_pipeline)
{
	int				wr;
	t_string		sep;
	t_hdoc			req;

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
		return (close(wr), free(sep.ctx), -1);
	((char *)sep.ctx)[sep.len] = '\0';
	req = create_heredoc((char *)sep.ctx,
			!contains_quotes(((t_ast_node *)curr->children.ctx)[1]),
			ft_strncmp(((t_ast_node *)curr->children.ctx)[0].token.start,
				"<<-", 3) == 0, is_pipeline);
	return (write_heredoc(state, wr, &req), free(sep.ctx), curr->redir_idx);
}

static void	append_to_exist_heredoc(t_shell *state, t_ast_node *curr,
										int shared_idx, bool is_pipeline)
{
	t_redir			*r;
	int				append_fd;
	t_string		sep;
	t_hdoc			req;

	r = (t_redir *)vec_idx(&state->redirects, shared_idx);
	if (!r || !r->fname)
		return ;
	curr->redir_idx = shared_idx;
	curr->has_redirect = true;
	append_fd = open(r->fname, O_WRONLY | O_APPEND);
	if (append_fd < 0)
		critical_error_errno_ctx(r->fname);
	sep = word_to_hrdoc_string(((t_ast_node *)curr->children.ctx)[1]);
	req = create_heredoc((char *)sep.ctx,
			!contains_quotes(((t_ast_node *)curr->children.ctx)[1]),
			ft_strncmp(((t_ast_node *)curr->children.ctx)[0].token.start,
				"<<-", 3) == 0,
			is_pipeline);
	(write_heredoc(state, append_fd, &req), free(sep.ctx));
}

void	process_redirect_group(t_shell *state, t_ast_node *parent,
								size_t start, size_t end)
{
	size_t		k;
	int			shared_idx;
	bool		is_pipeline;
	t_ast_node	*curr;
	t_token		tt;

	shared_idx = -1;
	is_pipeline = parent->node_type == AST_COMMAND_PIPELINE;
	k = start - 1;
	while (++k < end)
	{
		curr = &((t_ast_node *)parent->children.ctx)[k];
		if (curr->node_type != AST_REDIRECT)
			continue ;
		tt = ((t_ast_node *)curr->children.ctx)[0].token;
		if (tt.tt == TT_HEREDOC)
		{
			if (shared_idx == -1)
				shared_idx = create_heredoc_tempfile(state, curr, is_pipeline);
			else
				append_to_exist_heredoc(state, curr, shared_idx, is_pipeline);
		}
		else
			gather_heredoc(state, curr, is_pipeline);
	}
}

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
