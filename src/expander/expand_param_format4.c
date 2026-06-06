/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_format4.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 09:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 09:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/*
** Find suffix/prefix removal operators: %, %%, #, ##
** Returns pointer to the operator, or NULL if not found.
*/
static const char	*find_trim_op(const char *s, int slen, int *name_len)
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
		return (NULL);
	*name_len = i;
	if (i < slen && (s[i] == '%' || s[i] == '#'))
		return (&s[i]);
	return (NULL);
}

/*
** Find a pattern-substitution operator: ${name/pat/rep} or ${name//pat/rep}.
** Returns a pointer to the '/', or NULL if the spec is not a substitution.
*/
static const char	*find_subst_op(const char *s, int slen, int *name_len)
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
		return (NULL);
	*name_len = i;
	if (i < slen && s[i] == '/')
		return (&s[i]);
	return (NULL);
}

/*
** Main entry: expand a parameter format token.
** The token->start points to the content between ${ and }.
** e.g. for ${HOME:-/tmp}, start="HOME:-/tmp", len=10
** Returns allocated string, or NULL if not a special format.
*/
char	*expand_param_format(t_shell *state, const char *s, int slen)
{
	const char	*op;
	int			name_len;
	t_pe_op		o;
	t_trim_ctx	ctx;

	if (slen <= 0)
		return (ft_strdup(""));
	if (s[0] == '#' && slen > 1)
		return (expand_strlen(state, s + 1, slen - 1));
	if (find_param_op(s, slen, &o))
		return (expand_param_op(state, o));
	op = find_trim_op(s, slen, &name_len);
	if (!op)
		op = find_subst_op(s, slen, &name_len);
	if (op)
	{
		ctx.name = s;
		ctx.name_len = name_len;
		ctx.op = op;
		ctx.slen = slen;
		if (*op == '/')
			return (expand_subst(state, ctx));
		return (expand_trim(state, ctx));
	}
	return (NULL);
}
