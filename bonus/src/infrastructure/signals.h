/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   signals.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:11:08 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:11:08 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SIGNALS_H
# define SIGNALS_H

# include "common.h"

static inline void	default_signal_handlers(void)
{
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
}

static inline void	readline_bg_signals(void)
{
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_IGN);
}

static inline void	set_unwind(int sig)
{
	(void)sig;
	g_should_unwind = SIGINT;
}

static inline void	set_unwind_sig(void)
{
	struct sigaction	new_action;

	new_action = (struct sigaction){};
	new_action.sa_handler = set_unwind;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = SA_RESTART;
	sigaction(SIGINT, &new_action, NULL);
	signal(SIGQUIT, SIG_IGN);
}

static inline void	set_unwind_sig_norestart(void)
{
	struct sigaction	new_action;

	new_action = (struct sigaction){};
	new_action.sa_handler = set_unwind;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction(SIGINT, &new_action, NULL);
	signal(SIGQUIT, SIG_IGN);
}

# endif