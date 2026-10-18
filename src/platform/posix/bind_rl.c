/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bind_rl.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "zle.h"

/* The readline half of `bind` (builtin_bind.c parses the options).
**
** Every request is a readline call bash makes too, so the answers --
** bind -p's inputrc lines, bind -v's settings, -q's "can be invoked via"
** -- are readline's own, word for word. */

/* Readline built and every recorded request applied, as it is at a
   prompt -- so `bind '...'; bind -p` in a script lists what it just
   bound -- then -m's keymap made current. bash warns that line editing
   is off there, then answers. Returns the keymap to put back. */
static Keymap	bind_rl_ready(t_shell *state, const char *map)
{
	Keymap	km;

	if (state->metinp != INP_RL && !state->opt_interactive)
		ft_eprintf("%s: bind: warning: line editing not enabled\n",
			state->ctx);
	rl_preinit(&state->rl);
	km = rl_get_keymap();
	if (map && rl_get_keymap_by_name(map))
		rl_set_keymap(rl_get_keymap_by_name(map));
	return (km);
}

/* -p -P -v -V -s -S -l: readline's dumpers, onto stdout, in the keymap
   -m named (bind_rl_ready switched to it). */
int	bind_rl_list(t_shell *state, const char *map, char opt)
{
	FILE	*saved;
	Keymap	km;

	km = bind_rl_ready(state, map);
	saved = rl_outstream;
	rl_outstream = stdout;
	if (opt == 'p' || opt == 'P')
		rl_function_dumper(opt == 'p');
	else if (opt == 'v' || opt == 'V')
		rl_variable_dumper(opt == 'v');
	else if (opt == 's' || opt == 'S')
		rl_macro_dumper(opt == 's');
	else if (opt == 'l')
		rl_list_funmap_names();
	fflush(stdout);
	rl_outstream = saved;
	rl_set_keymap(km);
	return (0);
}

/* The first five, then "..." -- bash's query_bindings. The strings are
   readline's, from its own allocator, so they go back with free(). */
static void	bind_rl_print_keys(const char *name, char **keys)
{
	int	j;

	ft_printf("%s can be invoked via ", name);
	j = 0;
	while (j < 5 && keys[j])
	{
		ft_printf("\"%s\"", keys[j]);
		if (keys[j + 1])
			ft_printf(", ");
		else
			ft_printf(".\n");
		j++;
	}
	if (keys[j])
		ft_printf("...\n");
	j = 0;
	while (keys[j])
		free(keys[j++]);
	free(keys);
}

/* -q name: which keys run it, in -m's keymap. */
int	bind_rl_query(t_shell *state, const char *map, const char *name)
{
	t_zle_fn	f;
	char		**keys;
	Keymap		km;

	km = bind_rl_ready(state, map);
	f = rl_named_function(name);
	keys = NULL;
	if (f)
		keys = rl_invoking_keyseqs(f);
	rl_set_keymap(km);
	if (!f)
		return (ft_eprintf("%s: bind: `%s': unknown function name\n",
				state->ctx, name), 1);
	if (!keys || !keys[0])
	{
		free(keys);
		return (ft_printf("%s is not bound to any keys.\n", name), 1);
	}
	bind_rl_print_keys(name, keys);
	return (0);
}
