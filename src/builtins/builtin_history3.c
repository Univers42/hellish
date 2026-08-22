/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_history3.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/22 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/22 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "history.h"

/* history -s and history -p: the two options that take words rather than a
   file. Neither touches the history file. */

/* history -s arg... : append the ARGs to the list as ONE entry, joined by
   single spaces -- `history -s a b c` stores the single line "a b c", not
   three lines. No args is a silent success, as in bash. */
int	hist_store(t_shell *state, t_vec argv, int first)
{
	t_string	line;
	char		**av;

	if (first >= (int)argv.len)
		return (0);
	av = (char **)argv.ctx;
	vec_init(&line);
	line.elem_size = 1;
	while (first < (int)argv.len)
	{
		vec_push_str(&line, av[first++]);
		if (first < (int)argv.len)
			vec_push_char(&line, ' ');
	}
	vec_ensure_space_n(&line, 1);
	((char *)line.ctx)[line.len] = '\0';
	hist_push(state, (char *)line.ctx);
	return (0);
}

/* Expand one ARG the way the line editor would, but silently.

   expand_history() normally echoes a line it rewrote -- that is bash's
   "show me what !! became before you run it". Under -p the caller does the
   printing, and it prints even when nothing changed, so the echo has to be
   suppressed or `history -p '!!'` prints twice. hist_active is forced on
   for the call because a script has no session history, yet `history -s`
   may just have built a list that -p is entitled to see. */
static char	*hist_expand_quiet(t_shell *state, const char *arg)
{
	bool	live;
	char	*res;

	live = state->hist.hist_active;
	state->hist.hist_active = true;
	state->hist.quiet_expand = true;
	res = expand_history(state, arg);
	state->hist.quiet_expand = false;
	state->hist.hist_active = live;
	return (res);
}

/* history -p arg... : print each ARG after history expansion, storing
   nothing. A NULL result means the expander declined the string; the ARG
   is then its own expansion, which is what bash prints too. */
int	hist_expand_args(t_shell *state, t_vec argv, int first)
{
	char	**av;
	char	*res;

	av = (char **)argv.ctx;
	while (first < (int)argv.len)
	{
		res = hist_expand_quiet(state, av[first]);
		if (!res)
			ft_printf("%s\n", av[first]);
		else
		{
			ft_printf("%s\n", res);
			xfree(res);
		}
		first++;
	}
	return (0);
}
