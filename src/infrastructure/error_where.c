/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   error_where.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/03 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "lexer.h"
#include "sh_error.h"
#include "prompt.h"

/* Where a parse error is, the way bash says it for a sourced file.
**
** A syntax error in a student's ~/.zshrc came out as
**
**     hellish: syntax error near unexpected token `('
**     hellish: syntax error near unexpected token `('
**
** -- no file, no line, and twice (issue #113).  A dozen files load at
** startup (rc.d modules, the plugin framework, every plugin, the imported
** ~/.zshrc), so the report could not be acted on by the person reading it
** and could not be reproduced by anyone else.  Interactive bash says
**
**     bash: /home/u/.zshrc: line 12: syntax error near unexpected token `('
**     bash: /home/u/.zshrc: line 12: `[[ $x == (a|b) ]]'
**
** and that is the format here.  state->err_src is the file whose text is
** running: set by exec_file_string for `source` and the rc loader, NULL
** for eval, traps, -c and the REPL, whose ctx already says where.  The
** chunker replays a failed chunk statement by statement, so the token's
** line is the chunk's first file line (state->err_line, set by run_chunk)
** plus the newlines before the token inside the chunk.
*/

/* Owned: "ctx", or "hellish: FILE: line N" when a file is being sourced.
   `at` points into tt->base, the text the tokens index.  The shell's own
   name leads only when interactive -- bash prints "bash: FILE: line N" at
   a prompt and a bare "FILE: line N" from a script or -c. */
char	*parse_err_ctx(t_shell *state, t_deque_tok *tt, const char *at)
{
	int	line;

	if (!state->err_src || !tt || !tt->base || !at || at < tt->base)
		return (ft_strdup(state->ctx));
	line = state->err_line + nl_count(tt->base, (size_t)(at - tt->base));
	if (!state->opt_interactive)
		return (ft_asprintf("%s: line %d", state->err_src, line));
	return (ft_asprintf("%s: %s: line %d", state->dft_ctx, state->err_src,
			line));
}

/* The offending line itself, quoted on a second line as bash does.  Only
   for a file: an eval string has no line worth echoing, and the REPL
   already showed it. */
void	parse_err_echo(t_shell *state, t_deque_tok *tt, const char *at)
{
	const char	*b;
	const char	*e;
	char		*where;

	if (!state->err_src || !tt || !tt->base || !at || at < tt->base)
		return ;
	b = at;
	while (b > tt->base && b[-1] != '\n')
		b--;
	e = at;
	while (*e && *e != '\n')
		e++;
	where = parse_err_ctx(state, tt, at);
	ft_eprintf("%s: `%.*s'\n", where, (int)(e - b), b);
	xfree(where);
}
