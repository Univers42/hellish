/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   assignment_to_env2.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:08:32 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:08:32 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Continuation of assignment_to_env.c: the pre-store rewrites that a
   scalar assignment goes through before the t_env is committed — the +=
   append form (scalar_append) and the declare -n / -i attribute
   resolution (apply_var_attrs). */

#include "expander_private.h"
#include "sys.h"
#include "env.h"
#include "arith.h"

/* NAME+=value: the key still carries its trailing '+'. Prepend the
   variable's current value (string concatenation, bash scalar +=) and
   drop the '+'. A subscript key (a[i]+=) keeps its brackets for
   subscript_assign; only the plain-scalar case concatenates here. */
void	scalar_append(t_shell *state, t_env *ret)
{
	int		klen;
	char	*old;
	char	*joined;

	klen = (int)ft_strlen(ret->key);
	if (klen < 1 || ret->key[klen - 1] != '+' || !ret->value)
		return ;
	ret->key[klen - 1] = '\0';
	if (ft_strchr(ret->key, '['))
		return ;
	old = env_expand(state, ret->key);
	if (!old)
		old = "";
	joined = ft_strjoin(old, ret->value);
	xfree(ret->value);
	ret->value = joined;
}

/* Bare variable name in a key that may carry a trailing '+' (append) or
   a '[sub]' element: everything up to the first of those. */
static int	bare_name_len(const char *key)
{
	int	i;

	i = 0;
	while (key[i] && key[i] != '[' && key[i] != '+' && key[i] != '=')
		i++;
	return (i);
}

/* Integer attribute: replace the value with the arithmetic evaluation of
   the RHS (n="3*4" -> "12"). For += the new value is old+rhs. */
static void	apply_int_attr(t_shell *state, t_env *ret, int append)
{
	t_string	expr;
	char		*old;
	char		*res;

	vec_init(&expr);
	expr.elem_size = 1;
	old = env_expand(state, ret->key);
	if (append && old && *old)
	{
		vec_push_str(&expr, old);
		vec_push_char(&expr, '+');
	}
	vec_push_str(&expr, ret->value);
	vec_push_char(&expr, '\0');
	res = arith_expand(state, (char *)expr.ctx, (int)expr.len - 1);
	xfree(expr.ctx);
	xfree(ret->value);
	if (res)
		ret->value = res;
	else
		ret->value = ft_strdup("0");
}

/* Resolve declare -n / -i attributes on a scalar assignment. A nameref
   retargets the write to its target name (re-checked for int-ness);
   an integer attribute arithmetic-evaluates the RHS. Array-element and
   append+nameref combos fall through unchanged (v1). */
void	apply_var_attrs(t_shell *state, t_env *ret)
{
	int		nl;
	int		append;
	char	*tgt;
	char	*suffix;

	nl = bare_name_len(ret->key);
	tgt = attr_target(state, ret->key, nl);
	if (tgt && *tgt)
	{
		suffix = ft_strdup(ret->key + nl);
		xfree(ret->key);
		ret->key = ft_strjoin(tgt, suffix);
		xfree(suffix);
		nl = bare_name_len(ret->key);
	}
	if (attr_kind(state, ret->key, nl) != 'i' || ret->key[nl] == '[')
		return ;
	append = (ret->key[nl] == '+');
	if (append)
		ret->key[nl] = '\0';
	if (ret->value)
		apply_int_attr(state, ret, append);
}

/* The associative half: the subscript is a LITERAL string key, so it is
   word-expanded and used as written rather than evaluated. */
void	assoc_elem_assign(t_shell *state, t_env *ret, char *sub, char *old)
{
	char	*key;
	char	*nv;

	key = expand_param_word(state, sub, (int)ft_strlen(sub) - 1, false);
	nv = assoc_with_set(old, key, (int)ft_strlen(key), ret->value);
	xfree(key);
	xfree(ret->value);
	ret->value = nv;
}
