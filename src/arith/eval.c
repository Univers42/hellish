/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   eval.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:46:59 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"
#include "sh_input.h"

void	exit_clean(t_shell *state, int code);

/* Run the parser over an already-initialized lexer (scanning OR cached
   token-array mode). Shared by both arith_eval and arith_eval_cached so the
   parse logic lives in exactly one place. An empty expression (first token
   already EOF) returns 0 like bash. Any leftover tokens after arith_parse_expr
   returns are also an error -- e.g. "1 2" has no operator between the two. */
long long	arith_run(t_shell *state, t_arith_lexer *lexer, bool *error,
				bool nse)
{
	t_arith_parser	parser;
	long long		result;

	*error = false;
	parser.lexer = lexer;
	parser.shell = state;
	parser.error = false;
	parser.no_side_effects = nse;
	parser.error_msg = NULL;
	if (lexer->current.type == ATOK_EOF)
		return (0);
	result = arith_parse_expr(&parser);
	if (!parser.error && lexer->current.type != ATOK_EOF)
		parser.error = true;
	if (parser.error || lexer->error)
	{
		*error = true;
		return (0);
	}
	return (result);
}

/* Evaluate an arithmetic expression and return the numeric result.
   Sets *error to true on any lexer or parse error. The caller decides
   what to do with the error; arith_expand wraps this and calls arith_fail
   to print a message and possibly exit. */
long long	arith_eval(t_shell *state, const char *expr, int len, bool *error)
{
	t_arith_lexer	lexer;

	arith_lexer_init(&lexer, expr, len);
	return (arith_run(state, &lexer, error, false));
}

/* Convert a long long to a fresh malloc'd decimal string. The trick is casting
   to unsigned long long before negating so that LLONG_MIN's magnitude doesn't
   overflow (-LLONG_MIN overflows in signed arithmetic). We fill a local 32-byte
   buffer right-to-left then strdup the relevant slice -- no divide-and-recurse
   recursion, no sprintf dependency. */
char	*arith_lltoa(long long value)
{
	char				buf[32];
	int					i;
	int					neg;
	unsigned long long	u;

	neg = (value < 0);
	u = (unsigned long long)value;
	if (neg)
		u = -u;
	i = 31;
	buf[i] = '\0';
	if (u == 0)
		buf[--i] = '0';
	while (u > 0)
	{
		buf[--i] = '0' + (int)(u % 10);
		u /= 10;
	}
	if (neg)
		buf[--i] = '-';
	return (ft_strdup(buf + i));
}

/* Report an arithmetic error on stderr, set last_cmd_st_exe to 127, and exit
   if we're not in interactive readline mode (scripts, -c, piped input). For
   interactive use we keep going -- the error is already reported and the
   prompt will reappear. Returns NULL so callers can write
   `return (arith_fail(...))`.

   prompt_depth is the second exemption, and it is not a nicety: PS1 renders
   on every redraw, so `PS1='$((1/0))'` in a -c script or an rc file would
   exit the shell from inside the thing that draws the prompt -- leaving no
   session in which to fix the typo. bash reports and carries on with the
   text left literal, which is what the caller (ps1_arith) does with NULL. */
char	*arith_fail(t_shell *state, const char *expr, int len)
{
	ft_eprintf("%s: %.*s: arithmetic error\n", state->ctx, len, expr);
	state->last_cmd_st_exe = (t_execution_state){.status = 127};
	if (state->metinp != INP_RL && !state->prompt_depth)
		exit_clean(state, 127);
	return (NULL);
}

/* Evaluate expr and return the result as a fresh string suitable for word
   expansion. Calls arith_fail (which may exit) on error and returns NULL,
   so the caller must handle NULL. This is the non-cached path used for
   one-shot $((...)) expansions; hot paths use arith_expand_cached. */
char	*arith_expand(t_shell *state, const char *expr, int len)
{
	long long	result;
	bool		error;

	result = arith_eval(state, expr, len, &error);
	if (error)
		return (arith_fail(state, expr, len));
	return (arith_lltoa(result));
}
