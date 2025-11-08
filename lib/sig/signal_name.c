/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   signal_name.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:25:21 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 00:36:45 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <trap.h>

char	*signal_name(int sig)
{
	char	*ret;

	if (sig < 0 || sig >= BASH_NSIG || signal_names[sig] == NULL)
		ret = _("invalid signal number");
	else
		ret = signal_names[sig];
	return (ret);
}
