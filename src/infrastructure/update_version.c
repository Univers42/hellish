/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_version.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include "version.h"

/* Read one dotted version component, advancing past a trailing '.'. */
static int	next_num(const char **s)
{
	int	n;

	n = 0;
	while (**s >= '0' && **s <= '9')
	{
		n = n * 10 + (**s - '0');
		(*s)++;
	}
	if (**s == '.')
		(*s)++;
	return (n);
}

/* Compare two dotted versions (an optional leading 'v' is ignored). Returns a
   positive number if a is newer than b, 0 if equal, negative if older. */
int	hellish_version_cmp(const char *a, const char *b)
{
	int	i;
	int	x;
	int	y;

	if (*a == 'v' || *a == 'V')
		a++;
	if (*b == 'v' || *b == 'V')
		b++;
	i = -1;
	while (++i < 3)
	{
		x = next_num(&a);
		y = next_num(&b);
		if (x != y)
			return (x - y);
	}
	return (0);
}

/* `hellish --version`, shaped like `bash --version`: a first line a human
   or a script can grep, then where the build came from. The repo slug and
   the asset name are printed too because they are what the updater will
   actually contact -- a build whose slug points somewhere unexpected is
   worth seeing before it downloads anything.

   Exits 0 without running any startup file or reading stdin, so it stays
   usable from a package manager, a Dockerfile or a CI probe. */
void	print_version(void)
{
	ft_printf("hellish, version %s (%s)\n", HELLISH_VERSION, HELLISH_ASSET);
	ft_printf("Release channel: https://github.com/%s\n", HELLISH_REPO);
	ft_printf("An almost-POSIX shell, diffed byte-for-byte "
		"against bash --posix.\n");
}
