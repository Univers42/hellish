/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_zsh_opt.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 20:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 20:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* setopt / unsetopt / emulate -- the zsh option builtins.
**
** REGISTERED UNCONDITIONALLY, not behind zsh_mode(), and that is a
** deliberate line rather than an oversight.  A new NAME is additive: a bash
** script that never says `setopt` cannot tell the difference.  A changed
** MEANING for syntax that already parses is what has to be gated, which is
** why the expander's flags are and these are not.  `pretty` and `update`,
** both hellish-only builtins, already sit on this side of the line.
**
** The one name with any collision risk is `print`, which some ksh
** installations ship as /usr/bin/print; there is none on Linux and none on
** any platform this builds for, so it is called out here rather than paid
** for with machinery.
*/

/* zsh option names are case-insensitive and ignore underscores, so
   NO_GLOB, no_glob, noglob and NoGlob are one option.  Normalise to lower
   case with the underscores removed before any comparison, and strip a
   leading `no` into an inversion -- `setopt noglob` and `unsetopt glob` are
   the same request.  Returns the inverted-ness. */
static bool	zopt_norm(const char *in, char *out, size_t cap)
{
	size_t	i;
	size_t	n;

	n = 0;
	i = 0;
	while (in[i] && n + 1 < cap)
	{
		if (in[i] != '_')
			out[n++] = (char)ft_tolower((unsigned char)in[i]);
		i++;
	}
	out[n] = '\0';
	if (n > 2 && out[0] == 'n' && out[1] == 'o' && !zopt_bare(out))
	{
		ft_memmove(out, out + 2, n - 1);
		return (true);
	}
	return (false);
}

/* Apply one option.  `on` is the requested state after the `no` prefix has
   been folded in.  Three outcomes, and the third is the interesting one:
   mapped (we have an equivalent and set it), KNOWN-INERT (a real zsh option
   with no behaviour here -- accepted, listed by name in zopt_inert so it is
   documented rather than silent), or unknown (zsh's own message and status,
   and execution continues, because zsh does not abort either). */
static int	zopt_one(t_shell *state, const char *name, bool on)
{
	char	norm[64];
	bool	inv;

	inv = zopt_norm(name, norm, sizeof(norm));
	if (inv)
		on = !on;
	if (zopt_apply(state, norm, on))
		return (0);
	if (zopt_inert(norm))
		return (0);
	return (ft_eprintf("%s: setopt: no such option: %s\n",
			state->ctx, name), 1);
}

/* setopt [name ...] -- with no arguments zsh lists the options that differ
   from the default; there is no faithful list to give here (most of the
   roster is inert), so it reports nothing and succeeds rather than printing
   a roster that would be a fiction. */
int	builtin_setopt(t_shell *state, t_vec argv)
{
	size_t	i;
	int		rc;

	rc = 0;
	i = 1;
	while (i < argv.len)
		if (zopt_one(state, ((char **)argv.ctx)[i++], true))
			rc = 1;
	return (rc);
}

int	builtin_unsetopt(t_shell *state, t_vec argv)
{
	size_t	i;
	int		rc;

	rc = 0;
	i = 1;
	while (i < argv.len)
		if (zopt_one(state, ((char **)argv.ctx)[i++], false))
			rc = 1;
	return (rc);
}
