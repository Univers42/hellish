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

/* One row of "What's new in 3.2": the keys that reach a new feature, in
   code-yellow, then what it does, in muted grey -- the shape of "Getting
   started", so each can be tried before the prompt even appears. Every
   row fits the 32 columns the right column has on an 80-column terminal.
   Keys are padded to ten columns by hand (ft_snprintf pads bytes; an arrow
   is three bytes in one column); the ASCII keys are for terminals where
   header_use_unicode() says no. A NULL key ends a group: the always-on
   features, then the hint keys, which need HELLISH_AUTOSUGGEST.
   wiki/whats-new.md is the long form of this table and must stay in step
   with it; next release, replace both and bump HELLISH_HEADER_REV.
   Returns 0 at the end of a group. */
static int	news_row(char *dst, int n)
{
	static const char	*utf8[] = {"\xe2\x86\x91 \xe2\x86\x93       ",
		"make <TAB>", "bind -x   ", "bindkey   ", "${var@P}  ", "$SRANDOM  ",
		NULL, "\xe2\x86\x92 ^F End  ", "Alt-F     ", "Alt-N/P   ", NULL};
	static const char	*ascii[] = {"Up Down   ", "make <TAB>", "bind -x   ",
		"bindkey   ", "${var@P}  ", "$SRANDOM  ", NULL, "Right ^F  ",
		"Alt-F     ", "Alt-N/P   ", NULL};
	static const char	*what[] = {"history by prefix", "Makefile targets",
		"key runs a command", "zsh binds now work", "render as a prompt",
		"and $EPOCHREALTIME", NULL, "take the dim hint", "take one word",
		"older/newer hint"};
	const char			**keys;

	keys = ascii;
	if (header_use_unicode())
		keys = utf8;
	if (!keys[n])
		return (0);
	ft_snprintf(dst, 192, "  \033[38;5;223m%s\033[0m "
		"\033[38;5;245m%s\033[0m", keys[n], what[n]);
	return (1);
}

/* The news block: a salmon NEW badge on a cyan title, the always-on rows,
   then the switch for hints and the keys that take them. `k` walks the
   table across its group break; `m` the caller-owned backing rows. */
static void	news_block(const char **r, int *i, char nw[][192])
{
	int	k;
	int	m;

	r[(*i)++] = "\033[1;48;5;209;38;5;236m NEW \033[0m "
		"\033[1;38;5;81mWhat's new in 3.2\033[0m";
	r[(*i)++] = "";
	k = 0;
	m = 0;
	while (news_row(nw[m], k++))
		r[(*i)++] = nw[m++];
	r[(*i)++] = "";
	r[(*i)++] = "  \033[38;5;223mexport HELLISH_AUTOSUGGEST=1\033[0m";
	while (news_row(nw[m], k++))
		r[(*i)++] = nw[m++];
}

/* The right column: getting-started tips, what is new in this release,
   then a live version/update status footer. Backing strings are static
   (shown once). */
void	build_right(const char **r)
{
	static char	nw[9][192];
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
