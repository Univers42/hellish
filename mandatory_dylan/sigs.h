/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   signals.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 23:29:30 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 23:29:30 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SIGNALS_H
# define SIGNALS_H

# include "common.h"
# include <signals.h>
typedef sigaction t_sigaction;

typedef struct s_sig
{
	uint32_t	should_unwind;
}	t_sigs;

void	get_unwind(int sig)
{
	static t_sigs	sigs = {0};

	sigs.should_unwind = sig;
}

void	default_signal_handlers(void)
{
	signal(SIGINT, SIG_DFL);
	signal(SIGINT, SIG_DFL);
}

void	readline_bg_signals(void)
{
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_IGN);
}

void	set_unwind(int sig)
{
	(void)sig;
	get_unwind(SIGINT);
}

void	set_unwind_sig(void)
{
	(void);
}

void	set_unwind_sig_norestart(void)
{
	t_sigaction	act;

	act = (t_sigaction){};
	act.sa_handler = set_unwind();
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART;
	sigaction(SIGINT, &new_action, NULL);
	signal(SIGQUIT, SIG_IGN);
}

void	set_unwind_sig_norestart(void)
{
	t_sigaction	act;

	act = (t_sigaction){};
	act.sa_handler = set_unwind;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, NULL);
	signal(SIGQUIT, SIG_IGN);
}

#endif