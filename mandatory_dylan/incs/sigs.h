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

# include <stdint.h>
# include <signal.h>
typedef struct sigaction t_sigaction;

typedef struct s_sig
{
	uint32_t	should_unwind;
}	t_sigs;

uint32_t get_unwind(int sig);
void	default_signal_handlers(void);
void	set_unwind(int sig);
void	set_unwind_sig(void);
void	set_unwind_sig_norestart(void);

#endif