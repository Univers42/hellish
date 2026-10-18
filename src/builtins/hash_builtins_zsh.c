/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtins_repart.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:30 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:30 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* The zsh builtins.  Registered unconditionally rather than behind
   zsh_mode(): a new NAME is additive -- a bash script that never says
   `setopt` cannot tell -- while a changed MEANING for syntax that already
   parses is what has to be gated, which is why the expander's flags are.
   `pretty` and `update`, both hellish-only, already sit on this side of the
   line.  See src/builtins/builtin_zsh_opt.c. */
void	fill_builtin_hash5(t_hash *h);

void	fill_builtin_hash4(t_hash *h)
{
	hash_set(h, "setopt", (void *)builtin_setopt);
	hash_set(h, "unsetopt", (void *)builtin_unsetopt);
	hash_set(h, "emulate", (void *)builtin_emulate);
	hash_set(h, "print", (void *)builtin_print);
	hash_set(h, "autoload", (void *)builtin_autoload);
	hash_set(h, "zmodload", (void *)builtin_zunsupported);
	hash_set(h, "zstyle", (void *)builtin_zstyle);
	hash_set(h, "compdef", (void *)builtin_zunsupported);
	hash_set(h, "colors", (void *)builtin_colors);
	hash_set(h, "vcs_info", (void *)builtin_vcs_info);
	hash_set(h, "zle", (void *)builtin_zle);
	hash_set(h, "bindkey", (void *)builtin_bindkey);
	hash_set(h, "add-zsh-hook", (void *)builtin_add_zsh_hook);
	fill_builtin_hash5(h);
}

/* Programmable completion (#72 phase 4) and `bind`. Not zsh-only -- they
   live here because fill_builtin_hash4 is where the registration chain
   currently ends, and the norm caps a function at 25 lines. */
void	fill_builtin_hash5(t_hash *h)
{
	hash_set(h, "compgen", (void *)builtin_compgen);
	hash_set(h, "complete", (void *)builtin_complete);
	hash_set(h, "compopt", (void *)builtin_compopt);
	hash_set(h, "bind", (void *)builtin_bind);
}

/* The one exception to the rule above: names that are ALSO an external
   command. `which` is /usr/bin/which to a bash script -- `type which`,
   `command -v which` and its output format all depend on that -- and a
   zsh builtin to a zsh plugin (oh-my-zsh's sudo widget runs
   `builtin which -a`). An unconditional registration would change what
   the bash script runs, so these resolve only in zsh mode, through
   builtin_func(). */
int	(*zsh_builtin_func(char *name))(t_shell *s, t_vec argv)
{
	static t_hash	h = {0};

	if (!h.ctx)
	{
		hash_init(&h, 4);
		hash_set(&h, "which", (void *)builtin_which);
	}
	return ((t_builtin_fn)hash_get(&h, name));
}
