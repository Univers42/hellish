/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_trap3.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/19 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/19 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "executor.h"
#include <signal.h>

/* POSIX obsolescent reset form detector: `trap N ...` with N an unsigned
   decimal integer means "reset every operand", not "run the command N".
   bash's rule, matched exactly: all digits AND the value names a real
   signal (0..64).  So "07" resets SIGBUS but "100", " 42 " and "+2" are
   commands -- bash sets those as (useless) actions, it does not reset. */
int	trap_arg_is_reset(const char *s)
{
	int	val;

	if (!ft_isdigit((unsigned char)*s))
		return (0);
	val = 0;
	while (ft_isdigit((unsigned char)*s))
	{
		val = val * 10 + (*s++ - '0');
		if (val >= SH_NSIG)
			return (0);
	}
	return (*s == '\0');
}

/* Reset form: every operand from av[i] on is a condition restored to its
   default disposition.  A bad condition is reported and the walk continues
   -- bash still resets the valid ones and returns 1 (`trap - int 0 -99`
   resets INT and EXIT, then fails on -99). */
int	trap_reset_all(t_shell *state, t_vec argv, size_t i)
{
	char	**av;
	int		num;
	int		ret;

	av = (char **)argv.ctx;
	ret = 0;
	while (i < argv.len)
	{
		num = trap_sig_from_name(av[i]);
		if (num < 0)
		{
			ft_eprintf("%s: trap: %s: invalid signal\n", state->ctx, av[i]);
			ret = 1;
		}
		else
			set_one_trap(state, "-", num);
		i++;
	}
	return (ret);
}

/* Action form: av[i] is the action, everything after it a condition.  Same
   keep-going error policy as trap_reset_all, for the same bash-parity
   reason. */
int	trap_set_all(t_shell *state, t_vec argv, size_t i)
{
	char	**av;
	char	*action;
	int		num;
	int		ret;

	av = (char **)argv.ctx;
	action = av[i];
	ret = 0;
	while (++i < argv.len)
	{
		num = trap_sig_from_name(av[i]);
		if (num < 0)
		{
			ft_eprintf("%s: trap: %s: invalid signal\n", state->ctx, av[i]);
			ret = 1;
		}
		else
			set_one_trap(state, action, num);
	}
	return (ret);
}

/* POSIX: on entry to an asynchronous (&) child, traps are reset to their
   default actions -- the child must not run the parent's handlers when
   signalled (the SIGURG spec case: the parent traps URG, the & child must
   fall back to URG's default "ignore").  Traps set to "" stay ignored, so
   only non-empty actions get SIG_DFL; the strings are freed either way
   because this child's copy of the table no longer owns live traps. */
void	reset_traps_child(t_shell *state)
{
	int	i;

	xfree(state->traps[0]);
	state->traps[0] = NULL;
	i = 0;
	while (++i < SH_NTRAP)
	{
		if (i < SH_NSIG && state->traps[i] && state->traps[i][0] != '\0')
			pal_trap_dfl(state, i);
		xfree(state->traps[i]);
		state->traps[i] = NULL;
	}
}

/* One `trap -- 'action' COND` listing line.  Named signals print their
   short name (what bash --posix prints); the nameless realtime range falls
   back to the raw number, like dash. */
void	print_one_trap(t_shell *state, int num)
{
	char	*name;

	name = sig_to_name(num);
	if (name)
		ft_printf("trap -- '%s' %s\n", state->traps[num], name);
	else
		ft_printf("trap -- '%s' %d\n", state->traps[num], num);
}
