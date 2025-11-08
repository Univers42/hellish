/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   check_signals.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:00 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 05:33:31 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"
#include "quit.h"
/* Add any other shell timeouts here */
/* set by the read builtin */
void	check_signals(void)
{
	check_read_timeout ();
	quit();
}
