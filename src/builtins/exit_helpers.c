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

/* If the first operand is not a valid integer, print an error and exit with
   status 2. POSIX says "numeric argument required" and mandates that the
   shell exits immediately regardless of errexit — the exit happens inside
   exit_clean so we never return to the caller. */
int	handle_non_numeric(t_shell *state, t_vec argv, size_t i, int *ret)
{
	if (ft_checked_atoi(((char **)argv.ctx)[i], ret, 42))
	{
		ft_eprintf("%s: %s: %s: numeric argument required\n", state->ctx,
			((char **)argv.ctx)[0], ((char **)argv.ctx)[i]);
		return (exit_clean(state, 2), 1);
	}
	return (0);
}
