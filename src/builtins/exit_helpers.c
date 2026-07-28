/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exit_helpers.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 13:59:39 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:17:29 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <limits.h>

/* Parse an exit-status argument as a full long long, the way bash does:
   optional sign, digits, optional surrounding blanks, nothing else.  Unlike
   ft_checked_atoi (which clamps to int and is shared by kill/ulimit), this
   accepts the entire long long range so `exit 9223372036854775807` is a
   valid status that later gets masked to 8 bits.  A non-numeric operand or
   a value that overflows long long returns -1 -> "numeric argument
   required".  On success stores the value and returns 0.  The sign is
   consumed by a boolean increment (i += ...) — one line, not an if, to
   keep the body inside the 42-norm 25-line ceiling. */
int	exit_parse_ll(const char *s, long long *out)
{
	long long	n;
	int			i;
	int			neg;

	if (!s)
		return (-1);
	i = 0;
	while (s[i] == ' ' || s[i] == '\t')
		i++;
	neg = (s[i] == '-');
	i += (s[i] == '-' || s[i] == '+');
	if (!ft_isdigit(s[i]))
		return (-1);
	n = 0;
	while (ft_isdigit(s[i]))
	{
		if (n > (LLONG_MAX - (s[i] - '0')) / 10)
			return (-1);
		n = n * 10 + (s[i++] - '0');
	}
	while (s[i] == ' ' || s[i] == '\t')
		i++;
	if (s[i])
		return (-1);
	return (*out = n - 2 * n * neg, 0);
}

/* Print "exit" to stderr only for interactive (readline) sessions, matching
   bash/ksh behaviour — non-interactive scripts and -c invocations must not
   print it or every test harness would need to strip the word from output. */
void	print_exit_if_readline(t_shell *state)
{
	if (state->metinp == INP_RL)
		ft_eprintf("exit\n");
}

/* No explicit exit code: exit with the status of the last foreground command,
   just as `exit` with no arguments specifies in POSIX. */
int	handle_no_args(t_shell *state, t_vec argv)
{
	if (argv.len == 1)
	{
		exit_clean(state, state->last_cmd_st_exe.status);
		return (1);
	}
	return (0);
}

/* Skip an optional "--" separator. If "--" is the very last word (nothing
   after it), exit with the last-command status rather than an error — that
   is what bash does and scripts occasionally rely on it. */
size_t	handle_double_dash(t_shell *state, t_vec argv, size_t i)
{
	if (i < argv.len && ft_strcmp(((char **)argv.ctx)[i], "--") == 0)
	{
		if (i + 1 == argv.len)
			exit_clean(state, state->last_cmd_st_exe.status);
		return (i + 1);
	}
	return (i);
}

/* If the first operand is not a valid long long, print bash's "numeric
   argument required" and report failure (return 1).  bash does NOT exit the
   shell here -- it leaves $? = 2 and carries on -- so we return to the
   caller, which makes `exit` yield status 2, matching bash exactly. */
int	handle_non_numeric(t_shell *state, t_vec argv, size_t i, long long *ret)
{
	if (exit_parse_ll(((char **)argv.ctx)[i], ret))
	{
		ft_eprintf("%s: %s: %s: numeric argument required\n", state->ctx,
			((char **)argv.ctx)[0], ((char **)argv.ctx)[i]);
		return (1);
	}
	return (0);
}
