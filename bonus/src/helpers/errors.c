/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   errors.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:12 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:12 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../shell.h"
#include <errno.h>
#include <string.h>

void	critical_error(char *error)
{
	ft_eprintf("[ERROR] %s\n", error);
	exit(1);
}

void	critical_error_errno(void)
{
	ft_eprintf("[ERROR] %s\n", strerror(errno));
	exit(1);
}

void	critical_error_errno_context(char *context)
{
	ft_eprintf("[ERROR] %s: %s\n", context, strerror(errno));
	exit(1);
}

void	warning_error(char *error)
{
	ft_eprintf("[WARNING] %s\n", error);
}

void	warning_error_errno(void)
{
	ft_eprintf("[WARNING] %s\n", strerror(errno));
}
