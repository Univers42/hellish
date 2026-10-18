/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_bind3.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "zle.h"

/* bind -x '"KEYSEQ": COMMAND' -- a key that runs a shell command.
**
** The key becomes a widget of its own, named "bind -x KEYSEQ" (a name no
** `zle -N` can collide with), whose function is the command and which runs
** bash's way (zle_rl5.c: READLINE_LINE / _POINT / _MARK). Parsing follows
** bash's bind_keyseq_to_unix_command: the sequence must be double-quoted;
** a colon or blanks separate it from the command, which may be quoted with
** either quote. */

/* bash's isolate_sequence: from `i`, past blanks, one field -- delimited
   when it opens with a quote. Returns its end, *start its first byte, or
   -1 after saying why. The key sequence is the field at 0, and it alone
   must be double-quoted. */
static int	bind_isolate(t_shell *state, const char *s, int i, int *start)
{
	char	delim;
	bool	key;

	key = (i == 0);
	while (s[i] == ' ' || s[i] == '\t')
		i++;
	if (key && s[i] != '"')
		return (ft_eprintf("%s: bind: %s: first non-whitespace character "
				"is not `\"'\n", state->ctx, s), -1);
	delim = 0;
	if (s[i] == '"' || s[i] == '\'')
		delim = s[i++];
	*start = i;
	while (s[i] && s[i] != delim)
	{
		if (s[i] == '\\' && s[i + 1])
			i++;
		i++;
	}
	if (delim && s[i] != delim)
		return (ft_eprintf("%s: bind: no closing `%c' in %s\n",
				state->ctx, delim, s), -1);
	return (i);
}

static void	bind_x_add(const char *seq, const char *cmd)
{
	char			*name;
	t_zle_widget	*w;

	name = ft_strjoin("bind -x ", seq);
	if (!name)
		return ;
	zle_widget_add(name, cmd);
	w = zle_widget_get(name);
	if (w)
		w->bashx = true;
	zle_bind_add(seq, name);
	xfree(name);
}

/* -x LINE */
int	bind_unix(t_shell *state, const char *line)
{
	int		ks;
	int		i;
	int		cs;
	char	*seq;
	char	*cmd;

	i = bind_isolate(state, line, 0, &ks);
	if (i < 0)
		return (1);
	seq = ft_strndup(line + ks, (size_t)(i - ks));
	while (line[i] && line[i] != ':' && line[i] != ' ' && line[i] != '\t')
		i++;
	if (line[i] != ':' && line[i] != ' ' && line[i] != '\t')
		return (xfree(seq), ft_eprintf("%s: bind: %s: missing separator\n",
				state->ctx, line), 1);
	i = bind_isolate(state, line, i + 1, &cs);
	if (i < 0)
		return (xfree(seq), 1);
	cmd = ft_strndup(line + cs, (size_t)(i - cs));
	if (seq && cmd)
		bind_x_add(seq, cmd);
	return (xfree(seq), xfree(cmd), 0);
}

/* -X: the -x bindings, as bash prints them -- the command re-escaped so
   the line can be read back. */
int	bind_x_list(void)
{
	t_zle_widget	*a;
	size_t			i;
	size_t			j;

	a = (t_zle_widget *)zle_widgets()->ctx;
	i = -1;
	while (++i < zle_widgets()->len)
	{
		if (!a[i].bashx)
			continue ;
		ft_printf("\"%s\" \"", a[i].name + 8);
		j = -1;
		while (a[i].fn[++j])
		{
			if (a[i].fn[j] == '"' || a[i].fn[j] == '\\')
				ft_printf("\\");
			ft_printf("%c", a[i].fn[j]);
		}
		ft_printf("\"\n");
	}
	return (0);
}
