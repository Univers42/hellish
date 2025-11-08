/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   signal_is_async_ignored.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 01:04:49 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 06:19:07 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

int	signal_is_async_ignored(int sig)
{
	return (g_sig.sigmodes[sig] & SIG_ASYNCSIG);
}
