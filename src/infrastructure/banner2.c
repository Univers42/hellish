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

/* A rotating-feel showcase of real things this shell can do that a plain sh
   can't — a brag line plus two copy-pasteable tricks. Each is fronted by a
   coloured badge (g_chip) and carries its own inline colours: the command in
   warm code-yellow, the gloss in muted grey. All verified to work. */
static const char	*g_chip[] = {
	"\033[1;48;5;209;38;5;236m HOT \033[0m",
	"\033[1;48;5;141;38;5;236m PRO \033[0m",
	"\033[1;48;5;73;38;5;236m TIP \033[0m"
};

static const char	*g_news[] = {
	"\033[38;5;245moutpaces \033[1;38;5;209mbash --posix\033[0m"
	"\033[38;5;245m on compute-heavy scripts\033[0m",
	"\033[38;5;223mdiff <(a) <(b)\033[0m"
	"\033[38;5;245m  compare two outputs, no temp files\033[0m",
	"\033[38;5;223mcp report{,.bak}\033[0m"
	"\033[38;5;245m  duplicate a name in one word\033[0m",
	NULL
};

/* The block heading — an invitation to learn a trick, in cool cyan. */
static void	news_head(char *dst)
{
	ft_snprintf(dst, 160, "\033[1;38;5;81mDid you know?\033[0m");
}

/* One showcase line: an indented badge, then the self-coloured tip. */
static void	news_item(char *dst, const char *chip, const char *text)
{
	ft_snprintf(dst, 160, "  %s %s\033[0m", chip, text);
}

/* The showcase block: the heading, one badged tip per feature, then a salmon
   pointer onward to the docs. Backing strings are caller-owned. */
static void	news_block(const char **r, int *i, char nw[][160])
{
	const char	*arw;
	int			n;

	news_head(nw[0]);
	r[(*i)++] = nw[0];
	r[(*i)++] = "";
	n = -1;
	while (g_news[++n])
	{
		news_item(nw[n + 1], g_chip[n], g_news[n]);
		r[(*i)++] = nw[n + 1];
	}
	arw = "->";
	if (header_use_unicode())
		arw = "\xe2\x86\x92";
	ft_snprintf(nw[4], 160, "  \033[38;5;173m%s\033[0m "
		"\033[38;5;245mmore in docs/, or run \033[0m\033[38;5;223mhelp\033[0m",
		arw);
	r[(*i)++] = "";
	r[(*i)++] = nw[4];
}

/* The right column: getting-started tips, the "Did you know?" showcase, then a
   live version/update status footer. Backing strings are static (shown once).
*/
void	build_right(const char **r)
{
	static char	nw[5][160];
	static char	status[192];
	int			i;

	i = 0;
	r[i++] = HP_HEAD "Getting started";
	r[i++] = "";
	r[i++] = "  help       commands & options";
	r[i++] = "  update     check for a newer release";
	r[i++] = "  type NAME  how any name resolves";
	r[i++] = HP_RULE;
	news_block(r, &i, nw);
	r[i++] = HP_RULE;
	status_line(status, sizeof(status));
	r[i++] = status;
	r[i] = NULL;
}
