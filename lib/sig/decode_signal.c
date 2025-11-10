/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   decode_signal.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:05 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 14:56:13 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

static inline int	decode_numeric_signal(const char *string)
{
	intmax_t	sig;

	if (valid_number(string, &sig))
	{
		if (sig >= 0 && sig < NSIG)
			return ((int)sig);
		return (NO_SIG);
	}
	return (NO_SIG);
}

static inline int	decode_realtime_signal(const char *string, int flags)
{
	intmax_t	sig;

	if ((strncmp(string, "SIGRTMIN+", 9) == 0) || ((flags & DSIG_NOCASE)
			&& strncasecmp(string, "SIGRTMIN+", 9) == 0))
	{
		if (valid_number(string + 9, &sig) && sig >= 0
			&& sig <= SIGRTMAX - SIGRTMIN)
			return (SIGRTMIN + (int)sig);
		return (NO_SIG);
	}
	else if ((strncmp(string, "RTMIN+", 6) == 0) || ((flags & DSIG_NOCASE)
			&& strncasecmp(string, "RTMIN+", 6) == 0))
	{
		if (valid_number(string + 6, &sig) && sig >= 0
			&& sig <= SIGRTMAX - SIGRTMIN)
			return (SIGRTMIN + (int)sig);
		return (NO_SIG);
	}
	return (NO_SIG);
}

static inline int	match_signal_name(const char *string,
									const char *name,
									int flags)
{
	const char	*trimmed;

	if (name == NULL || name[0] == '\0')
		return (0);
	if (strncmp(name, "SIG", 3) == 0)
	{
		trimmed = name + 3;
		if ((flags & DSIG_NOCASE) && strcasecmp(string, trimmed) == 0)
			return (1);
		if (!(flags & DSIG_NOCASE) && strcmp(string, trimmed) == 0)
			return (1);
		if (!(flags & DSIG_SIGPREFIX))
			return (0);
	}
	if ((flags & DSIG_NOCASE) && strcasecmp(string, name) == 0)
		return (1);
	if (!(flags & DSIG_NOCASE) && strcmp(string, name) == 0)
		return (1);
	return (0);
}

static inline int	decode_named_signal(const char *string, int flags)
{
	int		sig;
	char	*name;

	sig = -1;
	while (++sig < BASH_NSIG_TOTAL)
	{
		name = get_g_sig()->signal_name[sig];
		if (match_signal_name(string, name, flags))
			return (sig);
	}
	return (NO_SIG);
}

int	decode_signal(const char *string, int flags)
{
	int	sig;

	sig = decode_numeric_signal(string);
	if (sig != NO_SIG)
		return (sig);
	sig = decode_realtime_signal(string, flags);
	if (sig != NO_SIG)
		return (sig);
	sig = decode_named_signal(string, flags);
	return (sig);
}
