/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander_redirect.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:14 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:14 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* parse optional leading fd from operator token (e.g. "2>") */
static int	parse_src_fd(t_tt tt, t_token op_tok)
{
	int		src_fd;
	char	*p;

	if (tt == TT_REDIRECT_LEFT)
		src_fd = 0;
	else
		src_fd = 1;
	p = op_tok.start;
	if (p && ft_isdigit((unsigned char)*p))
	{
		src_fd = 0;
		while (ft_isdigit((unsigned char)*p))
		{
			src_fd = src_fd * 10 + (*p - '0');
			p++;
		}
	}
	return (src_fd);
}

/* expand the filename (child index 1)
into a single string (transfer ownership) */
static char	*expand_redir_fname(t_shell *state, t_ast_node *curr)
{
	t_ast_node	*target;

	if (curr->children.len < 2)
		return (NULL);
	target = &((t_ast_node *)curr->children.ctx)[1];
	if (target->node_type == AST_PROC_SUB)
		return (expand_proc_sub(state, target));
	if (target->node_type == AST_WORD)
		return (expand_word_single(state, target));
	return (NULL);
}

/* create and commit a redir entry from AST node,
on success updates node->redir_idx */
static int	commit_redir(t_shell *state,
					t_ast_node *curr,
					t_tt tt,
					int src_fd)
{
	t_redir			new_redir;
	t_token_old		full_token;
	char			*fname;
	t_ast_node		*target;

	target = &((t_ast_node *)curr->children.ctx)[1];
	if (target->node_type == AST_WORD)
		full_token = get_old_token(*target);
	else
		full_token = init_token_old();
	fname = expand_redir_fname(state, curr);
	if (!create_redir_4(tt, fname, &new_redir, src_fd))
	{
		print_redir_err(state, full_token, fname);
		return (free(fname), -1);
	}
	curr->redir_idx = state->redirects.len;
	curr->has_redirect = true;
	vec_push(&state->redirects, &new_redir);
	return (0);
}

int	redirect_from_ast_redir(t_shell *state, t_ast_node *curr, int *redir_idx)
{
	t_token		op_tok;
	t_tt		tt;
	int			src_fd;

	ft_assert(curr->node_type == AST_REDIRECT);
	if (curr->has_redirect)
	{
		*redir_idx = curr->redir_idx;
		return (0);
	}
	op_tok = ((t_ast_node *)curr->children.ctx)[0].token;
	tt = op_tok.tt;
	src_fd = parse_src_fd(tt, op_tok);
	if (commit_redir(state, curr, tt, src_fd) < 0)
		return (-1);
	*redir_idx = curr->redir_idx;
	return (0);
}
