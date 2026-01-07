/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sigs.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 15:52:26 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 15:52:26 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../incs/sigs.h"

uint32_t get_unwind(int sig)
{
	static volatile uint32_t stored = 0;
	stored = (uint32_t)sig;
	return (stored);
}

void	default_signal_handlers(void)
{
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
}

void	set_unwind(int sig)
{
	(void)sig;
	(void)get_unwind(sig);
}

void	set_unwind_sig(void)
{
	/* no-op placeholder to keep API available */
}

/* keep a single correct norestart variant */
void	set_unwind_sig_norestart(void)
{
	t_sigaction	act;

	act = (t_sigaction){0};
	act.sa_handler = set_unwind;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, NULL);
	signal(SIGQUIT, SIG_IGN);
}
