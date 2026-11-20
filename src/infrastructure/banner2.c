/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   banner2.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/05 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "banner_private.h"
#include "shell.h"

/* One row of "What's new in 3.4": a name, in code-yellow, then what it
   is, in muted grey -- the shape of "Getting started". Every row fits the
   32 columns the right column has on an 80-column terminal; keys are
   padded to ten. No row names hellish: a new user never had it. Someone
   coming from it is told by the status footer, and only when their
   install still lacks the polish command (update_brand.c).
   wiki/whats-new.md is the long form of this table and must stay in step
   with it; next release, replace both and bump HELLISH_HEADER_REV.
   Returns 0 past the last row. */
static int	news_row(char *dst, int n)
{
	static const char	*keys[] = {".polishrc ", "POLISH_*  ", "rc.d/*.sh ",
		NULL};
	static const char	*what[] = {"your config file", "your settings",
		"loaded like .hsh"};

	if (!keys[n])
		return (0);
	ft_snprintf(dst, 192, "  \033[38;5;223m%s\033[0m "
		"\033[38;5;245m%s\033[0m", keys[n], what[n]);
	return (1);
}

/* The news block: a salmon NEW badge on a cyan title, then the rows.
   `m` indexes both the table and the caller-owned backing rows. */
static void	news_block(const char **r, int *i, char nw[][192])
{
	int	m;

	r[(*i)++] = "\033[1;48;5;209;38;5;236m NEW \033[0m "
		"\033[1;38;5;81mWhat's new in 3.4\033[0m";
	r[(*i)++] = "";
	m = 0;
	while (news_row(nw[m], m))
	{
		r[(*i)++] = nw[m];
		m++;
	}
}

/* The right column: getting-started tips, what is new in this release,
   then a live version/update status footer. Backing strings are static
   (shown once). */
void	build_right(const char **r)
{
	static char	nw[3][192];
	static char	status[192];
	int			i;

	i = 0;
	r[i++] = HP_HEAD "Getting started";
	r[i++] = "";
	r[i++] = "  help       commands & options";
	r[i++] = "  update     check for updates";
	r[i++] = "  type NAME  what a name runs";
	r[i++] = HP_RULE;
	news_block(r, &i, nw);
	r[i++] = HP_RULE;
	status_line(status, sizeof(status));
	r[i++] = status;
	r[i] = NULL;
}
