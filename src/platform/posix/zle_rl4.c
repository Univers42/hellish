/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zle_rl4.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"
#include "zle.h"
#include <readline/readline.h>

/* Built-in widgets: a key bound to one of the editor's own commands.
**
**     bindkey '^[[A' history-beginning-search-backward
**
** is how every zsh config asks for "Up searches the history for lines
** that start with what I typed" (#136). Only shell-function widgets used
** to work: the name above bound the key to the shared dispatcher, which
** found no widget by that name and did nothing -- the key went dead.
**
** Here the name is resolved to readline's own function and the key is
** bound to THAT, not to the dispatcher. It has to be direct: readline's
** history searches keep their place by checking that the previous command
** was themselves (rl_last_func), and a key routed through the dispatcher
** would make every press a new search for the line the last one found.
**
** zsh and readline name most of these alike; the table is the ones they do
** not. A name readline does not know, and a widget a plugin registered
** under a built-in's name (zsh-history-substring-search does exactly that),
** stay with the dispatcher. */
static const char	*zle_rl_name(const char *w)
{
	static const char	*map[][2] = {
	{"history-beginning-search-backward", "history-search-backward"},
	{"history-beginning-search-forward", "history-search-forward"},
	{"up-line-or-beginning-search", "history-search-backward"},
	{"down-line-or-beginning-search", "history-search-forward"},
	{"history-substring-search-up", "history-substring-search-backward"},
	{"history-substring-search-down", "history-substring-search-forward"},
	{"history-incremental-search-backward", "reverse-search-history"},
	{"history-incremental-search-forward", "forward-search-history"},
	{"up-line-or-history", "previous-history"},
	{"down-line-or-history", "next-history"},
	{"up-history", "previous-history"},
	{"down-history", "next-history"},
	{"expand-or-complete", "complete"},
	{"complete-word", "complete"},
	{"delete-char-or-list", "delete-char"},
	{NULL, NULL}};
	int					i;

	i = 0;
	while (map[i][0] && ft_strcmp(map[i][0], w))
		i++;
	if (map[i][0])
		return (map[i][1]);
	return (w);
}

/* readline's function for built-in widget `w`, or NULL. zsh's `.name`
   is the built-in version of a widget a plugin may have wrapped. */
t_zle_fn	zle_builtin_func(const char *w)
{
	if (!w || !*w)
		return (NULL);
	if (w[0] == '.')
		w++;
	return (rl_named_function(zle_rl_name(w)));
}

/* Bind one recorded `bindkey`: a widget the shell defined runs through
   the dispatcher, a built-in one is readline's own function. */
void	zle_bind_install(t_zle_bind *b)
{
	t_zle_fn	f;

	zle_bind_raw(b);
	f = NULL;
	if (!zle_widget_get(b->widget))
		f = zle_builtin_func(b->widget);
	if (!f)
		f = zle_dispatch;
	rl_bind_keyseq(b->seq, f);
}
