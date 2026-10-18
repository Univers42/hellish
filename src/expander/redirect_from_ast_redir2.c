/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   redirect_from_ast_redir2.c                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:14 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:14 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

int	get_default_src_fd(t_tt tt);
int	try_create_redir(t_shell *state, t_ast_node *curr, t_tt tt, int src_fd);

/* Read the fd number stored in the operator-token's leading digits (child 0
   of the redirect node).  The lexer packs "2>" as a token whose text starts
   with the '2'; we parse those digits and return the fd, falling back to the
   operator's default fd (0 for reads, 1 for writes) if no digits are found. */
int	get_src_fd(t_ast_node *curr, t_tt tt)
{
	t_ast_node	*first;
	const char	*s;
	int			i;
	int			fd;

	if (curr->children.len == 0)
		return (get_default_src_fd(tt));
	first = &((t_ast_node *)curr->children.ctx)[0];
	if (first->node_type != AST_TOKEN)
		return (get_default_src_fd(tt));
	s = first->token.start;
	i = 0;
	fd = 0;
	while (i < first->token.len && ft_isdigit((unsigned char)s[i]))
	{
		fd = fd * 10 + (s[i] - '0');
		i++;
	}
	if (i == 0)
		return (get_default_src_fd(tt));
	return (fd);
}

/* `n<&m` / `n>&m` whose m is not open in the shell, but is the target of an
   earlier redirect of the same command (`cmd 5<f <&5`, `cmd 6<f 7<&6`).
   Every redirect of a command is resolved before any is applied, so the
   fcntl probe in create_dup_redir saw fd 5 closed and refused a line bash
   runs. When the collector's pending list names m, the dup is built without
   the probe: by the time it is applied, m has been. The LAST redirect of m
   decides, so `cmd 5<f 5<&- <&5` is still refused. It used to pass for fd 3
   and 4 only because the shell happened to hold those itself. */
bool	dup_of_pending(t_shell *state, t_tt tt, char *fname, t_redir *ret)
{
	size_t	i;
	int		target;
	t_redir	*r;
	bool	open_at_apply;

	if (!state->pending_redirs || !state->pending_redirs->ctx || !fname
		|| (tt != TT_DUP_IN && tt != TT_DUP_OUT) || !dup_target_is_fd(fname))
		return (false);
	target = ft_atoi(fname);
	open_at_apply = false;
	i = 0;
	while (i < state->pending_redirs->len)
	{
		r = vec_idx(&state->redirects,
				(size_t)(*(int *)vec_idx(state->pending_redirs, i++)));
		if (r->src_fd == target)
			open_at_apply = !r->close_fd;
	}
	if (open_at_apply)
	{
		ret->fd = target;
		ret->close_fd = false;
		ret->is_dup = true;
	}
	return (open_at_apply);
}

/* A heredoc's backing (pipe or temp file) is registered by the heredoc module
   with stdin as its target, because that module never sees the operator's
   fd prefix.  Resolve the prefix here, where the other operators resolve
   theirs: `cmd 3<<EOF` used to feed the body to fd 0 and leave fd 3 closed,
   so `while read -r l <&3; do ...; done 3<<EOF` failed with "3: Bad file
   descriptor" and `cat 3<<EOF` printed a body bash never shows. */
static int	heredoc_redir(t_shell *state, t_ast_node *curr, t_token op_tok,
		int *redir_idx)
{
	t_redir	*r;

	if (curr->heredoc_body)
		materialize_heredoc(state, curr, redir_idx);
	else
	{
		ft_assert(curr->has_redirect);
		*redir_idx = curr->redir_idx;
	}
	r = vec_idx(&state->redirects, (size_t)(*redir_idx));
	r->src_fd = parse_src_fd(TT_HEREDOC, op_tok);
	return (0);
}

/* Top-level redirect processor for one AST_REDIRECT node.  Heredocs are
   handled specially: if the heredoc body is already materialised (has_redirect)
   we just return its cached index; if not yet materialised, materialize_heredoc
   is called.  All other redirect types go through try_create_redir which opens
   the file and registers the fd in state->redirects.  Either way the fd it
   acts on must be one a redirection can reach (redir_src_fd_ok). */
int	redirect_from_ast_redir(t_shell *state, t_ast_node *curr, int *redir_idx)
{
	t_token	op_tok;
	t_tt	tt;
	int		src_fd;

	ft_assert(curr->node_type == AST_REDIRECT);
	op_tok = ((t_ast_node *)curr->children.ctx)[0].token;
	tt = op_tok.tt;
	if (tt == TT_HEREDOC)
		heredoc_redir(state, curr, op_tok, redir_idx);
	else
	{
		src_fd = parse_src_fd(tt, op_tok);
		if (try_create_redir(state, curr, tt, src_fd) < 0)
			return (-1);
		*redir_idx = curr->redir_idx;
	}
	if (!redir_src_fd_ok(state,
			vec_idx(&state->redirects, (size_t)(*redir_idx))))
		return (-1);
	return (0);
}
