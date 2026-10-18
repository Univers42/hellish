/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_suggest3.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "zle.h"
#include "env.h"

/* The accept keys and POSTDISPLAY. */

/* What the key just pressed does when there is nothing to accept: the
   stock function of its accept slot (End goes to the end of the line, the
   rest move one character). */
static rl_command_func_t	*sg_stock(const char *keyseq)
{
	int	i;

	i = 0;
	while (keyseq && i < RL_NSLOTS)
	{
		if (rl_slot_at(i).feat == FEAT_ACCEPT && rl_slot_at(i).fn == SLOT_END
			&& !ft_strcmp(rl_slot_at(i).key, keyseq))
			return (rl_end_of_line);
		i++;
	}
	return (rl_forward_char);
}

/* Right, End and ^F while HELLISH_AUTOSUGGEST is on: take the ghost into
** the line when there is one, otherwise be the key's usual self. The
** whole suggestion is inserted, not just what was visible of it. */
int	rl_suggest_accept(int count, int key)
{
	t_shell		**cell;
	char		*g;

	cell = zle_state_cell();
	g = NULL;
	if (*cell)
		g = rl_suggest_live(&(*cell)->rl);
	if (g && *g)
	{
		rl_insert_text(g);
		return (0);
	}
	return ((*sg_stock(rl_executing_keyseq))(count, key));
}

/* Hand a widget the text after the cursor as the variable POSTDISPLAY, and
   remember what it was given to tell, afterwards, whether it was changed. */
void	rl_suggest_publish(t_shell *state)
{
	t_rl		*l;
	char		*g;

	l = &state->rl;
	g = rl_suggest_live(l);
	if (!g)
		g = "";
	xfree(l->sg_pub);
	l->sg_pub = ft_strdup(g);
	env_set(&state->env, env_create(ft_strdup("POSTDISPLAY"),
			ft_strdup(g), false));
}

/* A widget assigned POSTDISPLAY: show that, for the line as the widget
   left it. */
static void	sg_post_set(t_rl *l, const char *v)
{
	l->sg_txt = NULL;
	xfree(l->sg_post);
	xfree(l->sg_post_at);
	l->sg_post = ft_strdup(v);
	l->sg_post_at = ft_strdup(rl_line_buffer);
	rl_rprompt_hook();
}

/* After the widget, and after its BUFFER was installed (the ghost belongs
   to the new line): a POSTDISPLAY different from what was published is the
   widget's. */
void	rl_suggest_collect(t_shell *state)
{
	t_rl	*l;
	char	*v;

	l = &state->rl;
	v = env_expand(state, "POSTDISPLAY");
	if (!v)
		v = "";
	if (l->sg_pub && ft_strcmp(v, l->sg_pub))
		sg_post_set(l, v);
	xfree(l->sg_pub);
	l->sg_pub = NULL;
}
