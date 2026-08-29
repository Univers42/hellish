/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zsh_mode.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 17:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 17:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"

/* The zsh dialect gate.
**
** zsh syntax is not bash and is not POSIX: `${(f)x}`, `print -P`, `setopt`,
** `} always {` and unbraced `$arr[i]` all mean something else -- or nothing
** -- in the language the 3790 golden cases pin.  So none of it is reachable
** unless something says so.  Three things can:
**
**   set -o zsh / set +o zsh   explicit, scoped by hand
**   emulate zsh               what a plugin's own first line usually says
**   source foo.zsh            implicit, and the one that matters in practice
**
** The last is why plugins load without the user configuring anything: an
** oh-my-zsh plugin is `git.plugin.zsh`, and the extension is the only
** declaration of dialect that its author ever wrote down.  The mode is
** restored when the file finishes, so a .zsh plugin sourcing a .sh helper
** gets bash rules back for the helper, and a plugin cannot leave the
** interactive shell in a dialect the user never chose.
**
** Nothing here is a heuristic on file CONTENT.  Guessing the dialect from
** what the text looks like would make the same file parse differently
** depending on which line the scanner happened to notice first, which is
** exactly the class of silent wrongness the golden suite exists to prevent.
*/

bool	zsh_mode(t_shell *state)
{
	return (state && (state->setopt & SETOPT_ZSH) != 0);
}

/* Set the mode and hand back what it was, so the caller can restore it with
   a second call.  Callers pair these across a construct the way the parser
   pairs its own latches -- there is no stack because there is nothing to
   stack: the flag is one bit and the restore is unconditional. */
bool	zsh_mode_swap(t_shell *state, bool on)
{
	bool	was;

	if (!state)
		return (false);
	was = (state->setopt & SETOPT_ZSH) != 0;
	if (on)
		state->setopt |= SETOPT_ZSH;
	else
		state->setopt &= ~SETOPT_ZSH;
	return (was);
}

/* True for a path a zsh plugin would be shipped as.  `.zsh` covers both
   `foo.zsh` and oh-my-zsh's `foo.plugin.zsh`; `.zshrc` and `.zshenv` are
   matched by name because they carry no extension of their own.  A path
   ending in `.sh`, `.bash` or nothing at all is NOT zsh -- bash rules, which
   is the safe default when the author did not say. */
bool	zsh_path(const char *path)
{
	const char	*base;
	size_t		len;

	if (!path)
		return (false);
	base = ft_strrchr((char *)path, '/');
	if (base)
		base++;
	else
		base = path;
	len = ft_strlen(base);
	if (len >= 4 && !ft_strcmp(base + len - 4, ".zsh"))
		return (true);
	return (!ft_strcmp(base, ".zshrc") || !ft_strcmp(base, ".zshenv"));
}
