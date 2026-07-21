/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_case.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* ${v^} ${v^^} ${v,} ${v,,} ${v~} ${v~~}: bash case conversion.
   ^ upper, , lower, ~ toggle. Doubled operator converts the whole
   string; single converts only the first character. (The optional
   char-class pattern form ${v^^[abc]} is a documented v1 scope-out —
   bare-op case conversion is the near-universal usage.) */

/* Convert one char under op ('^' upper, ',' lower, '~' toggle). */
static char	case_conv(char c, char op)
{
	if (op == '^')
		return ((char)ft_toupper((unsigned char)c));
	if (op == ',')
		return ((char)ft_tolower((unsigned char)c));
	if (ft_isupper((unsigned char)c))
		return ((char)ft_tolower((unsigned char)c));
	return ((char)ft_toupper((unsigned char)c));
}

/* Apply the operator to an owned copy of the value: `all` converts every
   char, else just the first. */
char	*expand_case(t_shell *state, const char *s, int slen, int name_len)
{
	char	op;
	int		all;
	char	*val;
	int		i;

	op = s[name_len];
	all = (name_len + 1 < slen && s[name_len + 1] == op);
	val = pf_get_var_value(state, s, name_len);
	if (!val)
		return (ft_strdup(""));
	val = ft_strdup(val);
	if (!val || arr_is(val))
		return (val);
	i = 0;
	while (val[i])
	{
		val[i] = case_conv(val[i], op);
		if (!all)
			break ;
		i++;
	}
	return (val);
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
