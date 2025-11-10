/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   flag_compiler_helper.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 04:03:58 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 17:57:48 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

#ifdef SIGCHLD

void	init_chld_signal(void)
{
	get_orig_sig(SIGCHLD);
	get_g_sig()->sigmodes[SIGCHLD] |= (SIG_SPECIAL | SIG_NO_TRAP);
}
#else

void	init_chld_signal(void)
{
	return ;
}
#endif

#ifdef __BEOS__

void	init_int_signal(void)
{
	get_orig_sig(SIGINT);
	get_g_sig()->sigmodes[SIGINT] |= SIG_SPECIAL;
	get_g_sig()->original_signals[SIGINT] = SIG_DFL;
	get_g_sig()->sigmodes[SIGINT] &= ~SIG_HARD_IGNORE;
}
#else

void	init_int_signal(void)
{
	get_orig_sig(SIGINT);
	get_g_sig()->sigmodes[SIGINT] |= SIG_SPECIAL;
}

#endif