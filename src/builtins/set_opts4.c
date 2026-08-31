/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_opts4.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/19 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* The complete `set -o` roster, in bash's own listing order (alphabetical by
   name) so list_set_options can just walk it.  `letter` is '\0' for options
   with no short form.  `bit` is the e_setopt flag holding the state, or 0
   for the ten options that own a dedicated opt_* bool in t_shell -- those
   resolve through setopt_cell() below.

   Deliberately absent: `-r` (restricted).  bash accepts it; hellish does not
   implement a restricted mode, and quietly recording a security flag we do
   not enforce is worse than failing loudly, so `set -r` stays a usage error.

   `zsh` is the one entry bash does not have; setopt_hidden() (set_opts5.c)
   keeps it out of the `set -o` listing.  It gates the zsh dialect --
   parameter-expansion flags, the zsh builtins, `} always {` -- so none of
   that grammar can reach a script that never asked for it.  Sourcing a
   *.zsh file turns it on for that file (zsh_mode.c); `emulate zsh` and
   `set -o zsh` turn it on explicitly. */
const t_setopt	*setopt_table(void)
{
	static const t_setopt	tbl[] = {
	{"allexport", 'a', 0}, {"braceexpand", 'B', SETOPT_BRACEEXPAND},
	{"emacs", 0, SETOPT_EMACS}, {"errexit", 'e', 0},
	{"errtrace", 'E', SETOPT_ERRTRACE}, {"functrace", 'T', SETOPT_FUNCTRACE},
	{"hashall", 'h', SETOPT_HASHALL}, {"histexpand", 'H', SETOPT_HISTEXPAND},
	{"history", 0, SETOPT_HISTORY}, {"ignoreeof", 0, SETOPT_IGNOREEOF},
	{"interactive-comments", 0, SETOPT_ICOMMENTS},
	{"keyword", 'k', SETOPT_KEYWORD}, {"monitor", 'm', SETOPT_MONITOR},
	{"noclobber", 'C', 0}, {"noexec", 'n', 0}, {"noglob", 'f', 0},
	{"nolog", 0, SETOPT_NOLOG}, {"notify", 'b', SETOPT_NOTIFY},
	{"nounset", 'u', 0}, {"onecmd", 't', SETOPT_ONECMD},
	{"physical", 'P', SETOPT_PHYSICAL}, {"pipefail", 0, 0},
	{"posix", 0, 0}, {"privileged", 'p', SETOPT_PRIVILEGED},
	{"verbose", 'v', 0}, {"vi", 0, SETOPT_VI}, {"xtrace", 'x', 0},
	{"zsh", 0, SETOPT_ZSH},
	{NULL, 0, 0}};

	return (tbl);
}
