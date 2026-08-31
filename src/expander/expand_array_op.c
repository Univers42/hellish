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
int		try_unset(t_shell *state, char *key);

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
   element (whole value).
     The separator counts ELEMENTS, not bytes written. Keying it off
   out->len dropped the space whenever the operator emptied the elements
   before it: `a=(1 2); "${a[@]#1}"` is " 2" in bash (an empty first field,
   then 2) and was "2" here -- a field silently disappearing, which is
   worse than a wrong string because the count changes too.  nth[] pairs
   the counter with arr_next's length out-param to stay inside the norm's
   five declarations, the same shape emit_keys_fields uses. */
static void	array_op_loop(t_shell *state, const char *val, t_string *out,
				t_pe_op op)
{
	const char	*cur;
	const char	*v;
	long		idx;
	int			nth[2];
	char		*r;

	cur = "";
	if (arr_is(val))
		cur = val + 1;
	nth[0] = 0;
	while (arr_next(&cur, &idx, &v, &nth[1]))
	{
		r = array_op_elem(state, op, v, nth[1]);
		if (nth[0]++ > 0)
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

/* For an AGGREGATE subscript, which operators belong to this path: the
   default/alt/assign/error family (-, +, =, ? with an optional leading
   colon).  `${a[@]:1:2}` also starts with a colon and is a SLICE, owned by
   arr_slice further down the chain -- claiming it here turned
   `"${a[@]:1:2}"` into a substring of the joined string and produced " y"
   where bash gives "y z".  Element subscripts are unaffected: an operator
   is an operator there whatever it is. */
bool	at_op_ok(const char *op, int oplen)
{
	if (oplen < 1)
		return (false);
	if (ft_strchr("-+=?", op[0]))
		return (true);
	return (op[0] == ':' && oplen > 1 && ft_strchr("-+=?", op[1]) != NULL);
}
