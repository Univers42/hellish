/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   errors2.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:17 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:17 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include <errno.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"

void	err_cmd_not_found(t_shell *state, char *cmd)
{
	ft_eprintf("%s: %s: command not found\n", state->context, cmd);
}

void	err_no_path_var(t_shell *state, char *cmd)
{
	ft_eprintf("%s: %s: No such file or directory\n", state->context, cmd);
}

void	err_1_errno(t_shell *state, char *p1)
{
	ft_eprintf("%s: %s: %s\n", state->context, p1, strerror(errno));
}

void	err_2(t_shell *state, char *p1, char *p2)
{
	ft_eprintf("%s: %s: %s\n", state->context, p1, p2);
}

t_ast_node	unexpected(t_shell *state, t_parser *parser,
	t_ast_node ret, t_deque_tt *tokens)
{
	t_token	t;

	t = *(t_token *)deque_peek(&tokens->deqtok);
	if (ft_strncmp(t.start, "\n", t.len) == 0)
		ft_eprintf("%s: syntax error near unexpected token `newline'\n",
			state->context);
	else
		ft_eprintf("%s: syntax error near unexpected token `%.*s'\n",
			state->context, t.len, t.start);
	parser->res = RES_FatalError;
	return (ret);
}
