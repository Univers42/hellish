/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   shift_args.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <limits.h>

/* `shift` is a POSIX special builtin, so a MALFORMED request does not just
   set $? -- it aborts a non-interactive shell.  bash separates two failure
   kinds that both look like "shift returned non-zero", and the difference
   is not visible in the status:

     shift x     "numeric argument required"   status 2   ABORTS
     shift 1 2   "too many arguments"          status 1   ABORTS
     shift 99    "shift count out of range"    status 1   carries on

   The last one is a RESULT, not an error -- a script may legitimately ask
   to shift past the end and check $? -- so the status alone cannot decide
   it the way it does for export/readonly.  Hence the flag: we record the
   verdict and let finish_builtin() exit AFTER it has freed the command.
   Calling exit_clean() from here instead would abandon the live argv and
   word-slab frames, which ASan reports as a leak (see #78).  */
static void	shift_abort(t_shell *state)
{
	if (state->metinp != INP_RL)
		state->builtin_fatal = true;
}

/* A count wider than an int cannot be a valid shift on any positional list,
   so it is clamped rather than rejected: that lands it in the out-of-range
   branch, which is what bash reports for it. */
static int	shift_clamp(long long n)
{
	if (n > INT_MAX)
		return (INT_MAX);
	if (n < INT_MIN)
		return (INT_MIN);
	return ((int)n);
}

/* Read shift's operand.  Returns 0 with the count in *out, or bash's status
   after reporting (and, non-interactively, aborting).
     The number syntax is bash's legal_number -- surrounding blanks, an
   optional sign, digits, nothing else -- which exit_parse_ll already is, so
   `shift " 2 "` shifts two and `shift 0x2` is refused, both as bash does. */
int	shift_operand(t_shell *state, t_vec argv, int *out)
{
	long long	n;

	*out = 1;
	if (argv.len == 1)
		return (0);
	if (argv.len > 2)
		return (ft_eprintf("%s: shift: too many arguments\n", state->ctx),
			shift_abort(state), 1);
	if (exit_parse_ll(((char **)argv.ctx)[1], &n))
		return (ft_eprintf("%s: shift: %s: numeric argument required\n",
				state->ctx, ((char **)argv.ctx)[1]),
			shift_abort(state), 2);
	*out = shift_clamp(n);
	return (0);
}
