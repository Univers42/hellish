/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_dbracket3.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "env.h"
#include <regex.h>

/* [[ string =~ ERE ]] — POSIX extended regex via regcomp/regexec, with
   BASH_REMATCH populated as an indexed array: [0] the whole match,
   [1..n] the capture groups (unmatched groups become empty strings,
   like bash). Returns the test convention: 0 match, 1 no match, 2 for
   a regex that does not compile — bash's exact status for that. */

/* The evaluator recursion (db_or → ... → db_eval_flat) is state-free;
   eval_bracketed parks the t_shell here so the =~ primary can reach the
   environment. Same function-local-static pattern as anim_frame(). */
t_shell	**db_state_cell(void)
{
	static t_shell	*s;

	return (&s);
}

/* Build BASH_REMATCH from the regexec offsets. */
static void	rematch_store(t_shell *state, const char *s,
				regmatch_t *pm, int n)
{
	char	**elems;
	char	*val;
	int		i;

	elems = xmalloc(sizeof(char *) * (n + 1));
	if (!elems)
		return ;
	i = 0;
	while (i < n)
	{
		if (pm[i].rm_so >= 0)
			elems[i] = ft_strndup(s + pm[i].rm_so,
					pm[i].rm_eo - pm[i].rm_so);
		else
			elems[i] = ft_strdup("");
		i++;
	}
	val = arr_from_elems(elems, n, NULL);
	env_set(&state->env, env_create(ft_strdup("BASH_REMATCH"), val, false));
	i = 0;
	while (i < n)
		xfree(elems[i++]);
	xfree(elems);
}

int	db_regex_match(const char *str, const char *pat)
{
	regex_t		re;
	regmatch_t	pm[16];
	t_shell		*state;
	int			rc;
	int			n;

	if (regcomp(&re, pat, REG_EXTENDED) != 0)
		return (2);
	n = (int)re.re_nsub + 1;
	if (n > 16)
		n = 16;
	rc = regexec(&re, str, n, pm, 0);
	state = *db_state_cell();
	if (rc == 0 && state)
		rematch_store(state, str, pm, n);
	regfree(&re);
	if (rc == 0)
		return (0);
	return (1);
}
