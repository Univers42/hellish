/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cd_helpers1.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:21 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:21 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Unknown cd option: bash prints the offender then a usage line and exits 2.
   The shell's error filter only diffs the first line, so the message text
   ("... : invalid option") is what matters; the usage line mirrors bash. */
int	cd_invalid_opt(t_shell *state, const char *arg)
{
	ft_eprintf("%s: cd: %s: invalid option\n", state->ctx, arg);
	ft_eprintf("%s: cd: usage: cd [-L|[-P [-e]] [-@]] [dir]\n", state->ctx);
	return (2);
}

/* Apply one bundled option word (the chars after the leading '-'): -L picks
   logical resolution, -P physical (last of the two wins), -e/-@ are accepted
   for bash parity. Any other character makes the whole word invalid. */
static int	cd_parse_flags(const char *s, t_cdopt *o)
{
	int	i;

	i = 0;
	while (s[i])
	{
		if (s[i] == 'L')
			o->physical = false;
		else if (s[i] == 'P')
			o->physical = true;
		else if (s[i] == 'e')
			o->eflag = true;
		else if (s[i] == '@')
			o->atflag = true;
		else
			return (1);
		i++;
	}
	return (0);
}

/* Scan argv for options, which must precede the operand. A word that is not
   "-X..." (including "-", "", and redirection-looking words) ends the option
   run; "--" ends it explicitly and is consumed. *first is left pointing at the
   first operand. Returns 0, or 2 (already reported) on a bad option. */
int	cd_parse_opts(t_shell *state, t_vec argv, t_cdopt *o, size_t *first)
{
	size_t	i;
	char	*a;

	i = 1;
	while (i < argv.len)
	{
		a = ((char **)argv.ctx)[i];
		if (a[0] != '-' || a[1] == '\0')
			break ;
		if (!ft_strcmp(a, "--"))
		{
			i++;
			break ;
		}
		if (cd_parse_flags(a + 1, o))
			return (cd_invalid_opt(state, a));
		i++;
	}
	*first = i;
	return (0);
}
