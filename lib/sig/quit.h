/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   quit.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 01:38:58 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 05:50:34 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef QUIT_H
# define QUIT_H

# include "trap.h"

struct s_termg
{
	volatile sig_atomic_t	interrupt_state;
	volatile sig_atomic_t	terminating_signal;
	volatile sig_atomic_t	sigterm_received;
}	g_term = {0};

static inline void	quit(void)
{
	if (g_term.terminating_signal)
		termsig_handler(g_term.terminating_signal);
	else if (g_term.interrupt_state)
		throw_to_top_level();
}

static inline void	set_interrupt(void)
{
	g_term.interrupt_state = 1;
}

static inline void	clr_interrupt(void)
{
	g_term.interrupt_state = 0;
}

static inline void	add_interrupt(void)
{
	g_term.interrupt_state++;
}

void	zreset_if_needed(void);
void	check_sigterm(void);
#endif