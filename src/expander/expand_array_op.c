/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_array_op.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"
#include "parena.h"

bool	is_valid_ident(char *s, int len);
void	try_unset(t_shell *state, char *key);

#define AOP_VAR "__hellish_aop"

/* ${a[@]OP} where OP is a trim (#/##/%/%%) or substitution (/pat/rep):
   OP applies to EACH element and the results join with a space (echo/for
   parity; per-element field structure is the same v1 scope-out as
   slices). Rather than duplicate the pattern engine, each element is
   bound to a scratch variable and run through the ordinary scalar
   expand_param_format, so #, ##, %, %%, / and // all work for free.
   The scratch var is removed afterwards. */

/* Split "name[@]OP" / "name[*]OP": *nl = name length, *opat = offset of
   the operator byte just past "]". True only for a trim or subst op. */
static bool	array_op_split(const char *s, int len, int *nl, int *opat)
{
	int	i;

	i = 0;
	while (i < len && s[i] != '[')
		i++;
	if (i < 1 || i + 3 > len || (s[i + 1] != '@' && s[i + 1] != '*')
		|| s[i + 2] != ']')
		return (false);
	if (!is_valid_ident((char *)s, i))
		return (false);
	*nl = i;
	*opat = i + 3;
	if (*opat >= len)
		return (false);
	return (s[*opat] == '#' || s[*opat] == '%' || s[*opat] == '/');
}

/* Expand OP against one element (the [v, v+vl) slice) via the scratch
   var + scalar engine. Takes the raw slice rather than a C string so the
   caller does not need its own elem temporary. */
static char	*array_op_elem(t_shell *state, t_pe_op op, const char *v, int vl)
{
	char	*body;
	char	*res;
	int		kl;

	env_set(&state->env, env_create(ft_strdup(AOP_VAR),
			ft_strndup(v, vl), false));
	kl = (int)ft_strlen(AOP_VAR);
	body = xmalloc((size_t)kl + op.wlen + 1);
	ft_memcpy(body, AOP_VAR, kl);
	ft_memcpy(body + kl, op.word, op.wlen);
	body[kl + op.wlen] = '\0';
	res = expand_param_format(state, body, kl + op.wlen, false);
	xfree(body);
	if (!res)
		res = ft_strndup(v, vl);
	return (res);
}

/* Element loop: apply OP (op[0..oplen)) to each array element, joining
   results with a space into `out`. Only arrays iterate; a scalar is one
   element (whole value). */
static void	array_op_loop(t_shell *state, const char *val, t_string *out,
				t_pe_op op)
{
	const char	*cur;
	const char	*v;
	long		idx;
	int			vl;
	char		*r;

	cur = "";
	if (arr_is(val))
		cur = val + 1;
	while (arr_next(&cur, &idx, &v, &vl))
	{
		r = array_op_elem(state, op, v, vl);
		if (out->len)
			vec_push_char(out, ' ');
		vec_push_str(out, r);
		xfree(r);
	}
}

/* Iterate the array elements applying OP, space-joining the results. */
bool	expand_array_op(t_shell *state, t_token *tt)
{
	t_string	out;
	t_pe_op		op;
	char		*val;
	int			nl;
	int			opat;

	if ((tt->tt != TT_ENVVAR && tt->tt != TT_DQENVVAR)
		|| !array_op_split(tt->start, tt->len, &nl, &opat))
		return (false);
	val = env_expand_n(state, tt->start, nl);
	op.word = tt->start + opat;
	op.wlen = tt->len - opat;
	vec_init(&out);
	out.elem_size = 1;
	array_op_loop(state, val, &out, op);
	vec_push_char(&out, '\0');
	out.len--;
	try_unset(state, AOP_VAR);
	tt->start = (char *)out.ctx;
	tt->len = (int)out.len;
	tt->allocated = true;
	return (parena_note_attach(), true);
}
