/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   quit.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 01:38:58 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 18:37:26 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef QUIT_H
#define QUIT_H

#include "trap.h"

/* Forward declarations */
void termsig_handler(int sig);
void throw_to_top_level(void);

struct s_termg
{
	volatile sig_atomic_t interrupt_state;
	volatile sig_atomic_t terminating_signal;
	volatile sig_atomic_t sigterm_received;
};

/* Singleton accessor for global termination state */
struct s_termg *get_g_term(void);

static inline void quit(void)
{
	if (get_g_term()->terminating_signal)
		termsig_handler(get_g_term()->terminating_signal);
	else if (get_g_term()->interrupt_state)
		throw_to_top_level();
}

static inline void set_interrupt(void)
{
	get_g_term()->interrupt_state = 1;
}

static inline void clr_interrupt(void)
{
	get_g_term()->interrupt_state = 0;
}

static inline void add_interrupt(void)
{
	get_g_term()->interrupt_state++;
}

void zreset_if_needed(void);
void check_sigterm(void);
void check_read_timeout(void);

#endif
