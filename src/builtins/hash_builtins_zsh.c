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
void	fill_builtin_hash4(t_hash *h)
{
	hash_set(h, "setopt", (void *)builtin_setopt);
	hash_set(h, "unsetopt", (void *)builtin_unsetopt);
	hash_set(h, "emulate", (void *)builtin_emulate);
	hash_set(h, "print", (void *)builtin_print);
	hash_set(h, "autoload", (void *)builtin_autoload);
	hash_set(h, "zmodload", (void *)builtin_zunsupported);
	hash_set(h, "zstyle", (void *)builtin_zunsupported);
}
