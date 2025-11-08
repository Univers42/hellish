/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   signal_name.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:25:21 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 09:19:59 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"
#include <signal.h>

char	*signal_name(int sig)
{
	if (sig >= 0 && sig < BASH_NSIG_TOTAL && g_sig.signal_name[sig])
		return g_sig.signal_name[sig];
	if (sig > 0 && sig < NSIG)
		return (char *)strsignal(sig);
	return (char *)"invalid signal number";
}
