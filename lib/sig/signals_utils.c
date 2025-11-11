/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   signals_utils.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 18:20:41 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 18:22:29 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"


int	signal_is_special(int sig)
{
	return (get_g_sig()->sigmodes[sig] & SIG_SPECIAL);
}

int	signal_is_pending(int sig)
{
	return (get_g_sig()->pending_traps[sig]);
}

int	signal_is_ignored(int sig)
{
	return (get_g_sig()->sigmodes[sig] & SIG_IGNORED);
}

int	signal_is_hard_ignored(int sig)
{
	return (get_g_sig()->sigmodes[sig] & SIG_HARD_IGNORE);
}

int	signal_is_async_ignored(int sig)
{
	return (get_g_sig()->sigmodes[sig] & SIG_ASYNCSIG);
}
