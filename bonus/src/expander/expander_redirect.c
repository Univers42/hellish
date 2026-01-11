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

#include "../ft_printf/ft_printf.h"
#include "shell.h"
#include <fcntl.h>
#include <errno.h>
#include <string.h>

bool	create_redir(t_tt tt, char *fname, t_redir *ret)
{
	ft_assert(tt != TT_HEREDOC && "HEREDOCS are handled separately");
	ret->fname = fname;
	ret->direction_in = tt == TT_REDIRECT_LEFT;
	if (!ret->fname)
		return (false);
	if (tt == TT_REDIRECT_LEFT)
		ret->fd = open(ret->fname, O_RDONLY);
	else if (tt == TT_REDIRECT_RIGHT)
		ret->fd = open(ret->fname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	else if (tt == TT_APPEND)
		ret->fd = open(ret->fname, O_WRONLY | O_CREAT | O_APPEND, 0666);
	if (ret->fd < 0)
		return (false);
	ret->should_delete = false;
	return (true);
}

static void	print_redir_err(t_shell *state,
		t_token_old full_token, char *expanded_name)
{
	if (!expanded_name)
		ft_eprintf("%s: %.*s: ambigous redirect\n",
			state->context, full_token.len, full_token.start);
	else
		ft_eprintf("%s: %s: %s\n", state->context,
			expanded_name, strerror(errno));
}

int	redirect_from_ast_redir(t_shell *state, t_ast_node *curr, int *redir_idx)
{
	t_redir		new_redir;
	t_tt		tt;
	t_token_old	full_token;
	char		*fname;

	ft_assert(curr->node_type == AST_REDIRECT);
	if (curr->has_redirect)
	{
		*redir_idx = curr->redir_idx;
		return (0);
	}
	tt = ((t_ast_node *)curr->children.ctx)[0].token.tt;
	full_token = get_old_token(((t_ast_node *)curr->children.ctx)[1]);
	fname = expand_word_single(state, vec_idx(&curr->children, 1));
	if (!create_redir(tt, fname, &new_redir))
	{
		print_redir_err(state, full_token, fname);
		free(fname);
		return (-1);
	}
	curr->redir_idx = state->redirects.len;
	*redir_idx = state->redirects.len;
	curr->has_redirect = true;
	vec_push(&state->redirects, &new_redir);
	return (0);
}
