/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_xform.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* ${var@OP} parameter transformations (bash 5):
     @Q  single-quote the value so it can be re-read by the shell
     @U  uppercase   @L lowercase   @u uppercase-first
     @A  an assignment statement that would recreate the variable
   The rarer @E/@P/@a/@K are documented v1 scope-outs. */

/* @Q: wrap in single quotes, escaping any embedded single quote as the
   '\'' idiom bash uses (so the result re-parses to the original). */
static char	*xform_quote(const char *val)
{
	t_string	out;
	int			i;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_char(&out, '\'');
	i = 0;
	while (val[i])
	{
		if (val[i] == '\'')
			vec_push_str(&out, "'\\''");
		else
			vec_push_char(&out, val[i]);
		i++;
	}
	vec_push_char(&out, '\'');
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}

/* @U/@L/@u case transforms into a fresh buffer. */
static char	*xform_case(const char *val, char op)
{
	char	*r;
	int		i;

	r = ft_strdup(val);
	if (!r)
		return (NULL);
	i = 0;
	while (r[i])
	{
		if (op == 'U')
			r[i] = (char)ft_toupper((unsigned char)r[i]);
		else if (op == 'L')
			r[i] = (char)ft_tolower((unsigned char)r[i]);
		else if (op == 'u' && i == 0)
			r[i] = (char)ft_toupper((unsigned char)r[i]);
		i++;
	}
	return (r);
}

/* @A: "name='value'" assignment form (single-quoted value). */
static char	*xform_assign(const char *name, int nlen, const char *val)
{
	t_string	out;
	char		*q;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_nstr(&out, (char *)name, nlen);
	vec_push_char(&out, '=');
	q = xform_quote(val);
	vec_push_str(&out, q);
	xfree(q);
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}

/* Detect ${name@OP} and apply. name_len is the variable-name length,
   op the transform letter (the char after '@'). */
char	*expand_xform(t_shell *state, const char *s, int name_len, char op)
{
	char	*val;

	val = pf_get_var_value(state, s, name_len);
	if (!val)
		val = "";
	if (arr_is(val))
		val = "";
	if (op == 'Q')
		return (xform_quote(val));
	if (op == 'U' || op == 'L' || op == 'u')
		return (xform_case(val, op));
	if (op == 'A')
		return (xform_assign(s, name_len, val));
	return (ft_strdup(val));
}

/* Is s a ${name@OP} form? Sets *nl to name length and *op to the letter. */
bool	find_xform_op(const char *s, int slen, int *nl, char *op)
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
	if (i + 1 >= slen || s[i] != '@')
		return (false);
	*nl = i;
	*op = s[i + 1];
	return (ft_strchr("QULuAEPaKk", *op) != NULL);
}
