/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_case2.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "case_match.h"

/* Does one character -- the n bytes at c -- take part in a ${v^pat}
   conversion? Every character does when there is no pattern (bash's
   default is `?`); otherwise the whole character has to match it, and a
   pattern that could only match more than one never does. */
bool	case_hit(const char *c, size_t n, const char *pat)
{
	if (!pat || !*pat)
		return (true);
	return (case_match_n(c, n, pat, ft_strlen(pat)));
}

/* Is s a ${name^...} / ${name,...} / ${name~...} form? Sets *nl to the
   name length and returns true. Rejects the substring ':' cases and the
   trim/subst ops (those are matched earlier by their own finders). */
bool	find_case_op(const char *s, int slen, int *nl)
{
	int	i;

	i = 0;
	if (i < slen && ft_isdigit((unsigned char)s[i]))
		while (i < slen && ft_isdigit((unsigned char)s[i]))
			i++;
	else if (i < slen && (s[i] == '_' || ft_isalpha((unsigned char)s[i])))
	{
		i++;
		while (i < slen && (s[i] == '_' || ft_isalnum((unsigned char)s[i])))
			i++;
	}
	else
		return (false);
	*nl = i;
	return (i < slen && (s[i] == '^' || s[i] == ',' || s[i] == '~'));
}
