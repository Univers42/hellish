/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pal_proc2.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/28 16:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "pal.h"
#include <signal.h>

/* Trap-disposition shims.  POSIX arms the kernel handler directly; the
   win32 sibling instead records armed signal numbers in a bitmap that
   the console-control handler consults, because Windows delivers
   console events on their own thread rather than as signals. */
void	pal_trap_arm(t_shell *st, int sig, void (*handler)(int))
{
	(void)st;
	signal(sig, handler);
}

void	pal_trap_dfl(t_shell *st, int sig)
{
	(void)st;
	signal(sig, SIG_DFL);
}

void	pal_trap_ign(t_shell *st, int sig)
{
	(void)st;
	signal(sig, SIG_IGN);
}
