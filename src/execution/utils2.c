/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 16:50:27 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/24 19:27:52 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "sys.h"

void	set_up_redirection(t_shell *state, t_executable_node *exe)
{
	if (exe->next_infd != -1)
		close(exe->next_infd);
	if (exe->outfd != 1)
	{
		dup2(exe->outfd, 1);
		close(exe->outfd);
	}
	if (exe->infd != 0)
	{
		dup2(exe->infd, 0);
		close(exe->infd);
	}
	if (exe->redirs.len == 0)
		return ;
	if (exe->redirs.ctx)
	{
		apply_redirs_from_vec(state, exe);
		return ;
	}
	if (exe->node)
	{
		apply_redirs_from_ast(state, exe);
		return ;
	}
	if (state->context)
		ft_eprintf(MSG_REDIR_DATA_ERR, state->context);
	else
		ft_eprintf(MSG_REDIR_DATA_ERR, "shell");
	exit(EXIT_GENERAL_ERR);
}
