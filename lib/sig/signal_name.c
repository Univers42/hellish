/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   signal_name.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:25:21 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 19:28:36 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"
#include <signal.h>

char *signal_name(int sig)
{
	/* Prefer cached names within bounds of signal_name[] */
	if (sig >= 0 && sig < NSIG && get_g_sig()->signal_name[sig])
		return (get_g_sig()->signal_name[sig]);
	/* Return OS-provided name for real signals */
	if (sig > 0 && sig < NSIG)
		return ((char *)strsignal(sig));
	/* Handle custom trap identifiers safely without indexing array */
	if (sig == EXIT_TRAP)
		return ("EXIT");
	if (sig == DEBUG_TRAP)
		return ("DEBUG");
	if (sig == ERROR_TRAP)
		return ("ERROR");
	if (sig == RETURN_TRAP)
		return ("RETURN");
	return ((char *)"invalid signal number");
}
