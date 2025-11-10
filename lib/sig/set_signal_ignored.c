/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_signal_ignored.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:47 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 14:45:18 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	set_signal_ignored(int sig)
{
	get_g_sig()->original_signals[sig] = SIG_IGN;
}
