/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   signal_is_hard_ignored.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:56 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 06:18:16 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

int	signal_is_hard_ignored(int sig)
{
	return (g_sig.sigmodes[sig] & SIG_HARD_IGNORE);
}
