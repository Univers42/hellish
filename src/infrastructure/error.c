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
#include "sh_error.h"

/* These helpers centralise the exact wording of runtime error messages so the
   whole shell speaks with one voice. The format always starts with state->ctx
   (e.g. "hellish", or "hellish: line 4:" in script mode) to match bash. */

/* "cmd: command not found" — exit 127 in POSIX. */
void	err_cmd_not_found(t_shell *state, char *cmd)
{
	ft_eprintf("%s: %s: command not found\n", state->ctx, cmd);
}

/* "$PATH" is unset so we cannot look up the command at all -- same message as
   bash when the binary simply does not exist on any path component. */
void	err_no_path_var(t_shell *state, char *cmd)
{
	ft_eprintf("%s: %s: No such file or directory\n", state->ctx, cmd);
}

/* "ctx: p1: <system error>" — for syscall failures on a named target (file,
   command, etc.) where errno carries the reason. */
void	err_1_errno(t_shell *state, char *p1)
{
	ft_eprintf("%s: %s: %s\n", state->ctx, p1, strerror(errno));
}

/* "ctx: p1: p2" — two-field message when the reason is already a string
   rather than an errno code. */
void	err_2(t_shell *state, char *p1, char *p2)
{
	ft_eprintf("%s: %s: %s\n", state->ctx, p1, p2);
}

/* Report a syntax error and mark the parser as failed. The token under the
   cursor is shown verbatim, with "newline" substituted for the invisible '\n'
   token to match bash's wording exactly ("near unexpected token `newline'").
   The return value is whatever `ret` the caller passed in, so callers can
   write: return (unexpected(state, parser, empty_node, tokens)); */
t_ast_node	unexpected(t_shell *state, t_parser *parser,
	t_ast_node ret, t_deque_tok *tokens)
{
	t_token	t;
	char	*where;

	t = ltok2tok(*(t_ltoken *)deque_peek(&tokens->deqtok), tokens->base);
	parser->res = RES_ERR;
	parser->reported = !parser->quiet;
	if (parser->quiet)
		return (ret);
	where = parse_err_ctx(state, tokens, t.start);
	if (ft_strncmp(t.start, "\n", t.len) == 0)
		ft_eprintf("%s: syntax error near unexpected token `newline'\n",
			where);
	else
		ft_eprintf("%s: syntax error near unexpected token `%.*s'\n",
			where, t.len, t.start);
	xfree(where);
	parse_err_echo(state, tokens, t.start);
	return (ret);
}
