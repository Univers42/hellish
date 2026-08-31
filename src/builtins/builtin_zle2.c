/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_zle2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "zle.h"

/* The OPTION half of `zle`, and the reason it is now a function.
**
** It used to be two lines:
**
**     if (av[1][0] == '-')
**         return (0);
**
** -- every option but `-N` silently succeeded. `zle -M "hello"` returned 0
** and printed nothing; so did `zle -R`, `zle -F`, `zle -K` and `zle -U`.
** That is the exact failure the widget-name path in builtin_zle.c goes out
** of its way to avoid ("a widget that appears to run and does nothing is
** indistinguishable from a working one"), reached through the options
** instead. The plugin believes the message was shown and the user has
** nothing anywhere to read.
**
** `-R` is real here and is implemented rather than reported (`-M` is too,
** but it takes the message words and lives in builtin_zle.c):
**
**     zle -R         refresh the display        -- the same redisplay
**                                                  `zle redisplay` runs
**
** `-R`'s optional status-line argument is NOT shown; readline has no status
** line, and `zle -R` with no argument is the form plugins call on every
** keystroke. Saying so once here is the honest version -- complaining on
** every keystroke would be worse than the gap.
**
** Everything else says so, ONCE, and fails. Once because a plugin may call
** the same option per keystroke, and a diagnostic that repeats forever is
** one the user learns to ignore -- which is silence with extra steps.
*/

/* Has this option already been reported? One flag for the whole family
   rather than one per name: a session that has been told ZLE is partial has
   been told, and the second message adds nothing. */
static bool	zle_opt_told(void)
{
	static bool	told;

	if (told)
		return (true);
	told = true;
	return (false);
}

/* zle -R / anything else (-N and -M were dispatched before this). `i` is
   the index of the option word. */
int	zle_option(t_shell *state, t_vec argv, size_t i)
{
	char	**av;

	av = (char **)argv.ctx;
	if (!ft_strcmp(av[i], "-R"))
		return (zle_do_redisplay(), 0);
	if (!zle_opt_told())
		ft_eprintf("%s: zle: %s is not supported; ZLE here is readline "
			"with a zsh accent, see #77\n", state->ctx, av[i]);
	return (1);
}
