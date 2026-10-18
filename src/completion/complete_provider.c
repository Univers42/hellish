/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_provider.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 18:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 18:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Per-command argument completion providers.  cmd_completion calls
   comp_provider_try for an ARGUMENT word that no `complete` spec claimed;
   the table in comp_provider_try maps a command name to the function that
   knows its arguments, so a new provider is one row there and one function
   elsewhere.  A provider answers NULL for "not mine", which leaves
   readline's filename completion in charge exactly as before. */

#include "complete_make.h"
#include <readline/readline.h>

/* Words that only introduce the real command (`sudo make`, `X=1 make`,
   `env -i make`) are skipped so the provider sees the command that owns the
   argument.  An option after a wrapper is skipped too, but not its value:
   `sudo -u root make` is not understood and keeps filename completion. */
static int	prov_skip(const char *w, size_t n, int first)
{
	static const char	*wrap[] = {"sudo", "env", "command", "exec",
		"nohup", "time", "nice", "doas", NULL};
	int					i;

	if (!first && w[0] == '-')
		return (1);
	if ((ft_isalpha(w[0]) || w[0] == '_') && ft_memchr(w, '=', n))
		return (1);
	i = -1;
	while (wrap[++i])
	{
		if (ft_strlen(wrap[i]) == n && !ft_strncmp(w, wrap[i], n))
			return (1);
	}
	return (0);
}

/* The command word owning the argument being completed, past wrappers. */
static size_t	prov_cmd(t_pv_line *p, const char **w)
{
	size_t	n;
	int		first;

	first = 1;
	n = prov_word(p, w);
	while (n && prov_skip(*w, n, first))
	{
		first = 0;
		n = prov_word(p, w);
	}
	return (n);
}

/* The provider for the command named by w[0..n), or NULL. */
static t_prov_fn	prov_find(const char *w, size_t n)
{
	static const t_provider	tab[] = {{"make", comp_make},
	{"gmake", comp_make}, {NULL, NULL}};
	int						i;

	i = -1;
	while (n && tab[++i].cmd)
	{
		if (ft_strlen(tab[i].cmd) == n && !ft_strncmp(w, tab[i].cmd, n))
			return (tab[i].fn);
	}
	return (NULL);
}

/* Dispatch an argument word to the provider of its command, or NULL. */
char	**comp_provider_try(const char *text, int start, int end)
{
	t_shell		*st;
	t_pv_line	p;
	const char	*w;
	size_t		n;
	t_prov_fn	fn;

	(void)end;
	st = *zle_state_cell();
	if (!st || !rl_line_buffer || start <= 0)
		return (NULL);
	p.l = rl_line_buffer;
	p.pos = pc_cmd_off(start);
	p.end = (size_t)start;
	n = prov_cmd(&p, &w);
	fn = prov_find(w + prov_base(w, n), n - prov_base(w, n));
	if (!fn)
		return (NULL);
	return (fn(st, text, &p));
}
