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
#include "ft_glob.h"

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
** restored when the file finishes, so a plugin cannot leave the interactive
** shell in a dialect the user never chose.
**
** A .zsh file that sources a .sh helper KEEPS the zsh dialect for it: the
** extension decides when to ARM the mode, never when to disarm it.  That is
** what zsh itself does -- the dialect is a property of the shell, not of the
** file -- and a plugin's own helper is written for the plugin's dialect, so
** reverting on the way in would break the common case to honour a naming
** convention the helper's author never opted into.
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
   a second call.
     Also mirrors it into the glob layer's cell (glob_opts.c), which has no
   t_shell of its own -- the same arrangement nullglob and dotglob already
   use. Doing it HERE, in the one function that changes the mode, is what
   makes the mirror impossible to forget.
     Callers pair these across a construct the way the parser pairs its own
   latches -- there is no stack because there is nothing to stack: the flag
   is one bit and the restore is unconditional. */
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
	*glob_zsh_cell() = on;
	return (was);
}

/* Do arrays count from 1?  Only in the zsh dialect, and only while
   `ksh_arrays` is unset -- that option is zsh's own switch back to 0-based,
   and plugins write `setopt localoptions no_ksh_arrays` precisely to be sure
   of the default.  Accepting that line while ignoring what it names would
   make the reassurance a lie, so the option is real. */
bool	zsh_arrays(t_shell *state)
{
	return (state && (state->setopt & SETOPT_ZSH)
		&& !(state->setopt & SETOPT_KSHARRAYS));
}

/* Turn a WRITTEN subscript into the 0-based index the array store uses.
**
** zsh counts from 1 where bash counts from 0, and the difference is not
** noisy: `${a[$#a]}` -- the last element, which is how a plugin pops a stack
** -- returns a DIFFERENT ELEMENT under the wrong base and reports nothing at
** all.  Every subscript site used to do its own ft_atoi, so getting this
** right meant getting it right in four places; it is one function now for
** the same reason the dotglob cell is one cell.
**
** NEGATIVE subscripts are identical in both dialects: -1 is the last element
** either way.  They wrap against the count before the base applies, which is
** why the wrap is here and not left to the callers that used to do it.
*/
long	sub_to_index(t_shell *state, long sub, long count)
{
	if (sub < 0)
		return (sub + count);
	if (zsh_arrays(state))
		return (sub - 1);
	return (sub);
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
