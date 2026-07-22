/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   assignment_to_env.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:08:32 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:08:32 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sys.h"
#include "env.h"
#include "arith.h"

/* Copy the assignment LHS (variable name) out of the first child token of
   an AST_ASSIGNMENT_WORD node.  The first child is always a TT_WORD whose
   content is the bare name without the trailing '='. */
static char	*dup_key_from_node(t_ast_node *node)
{
	return (ft_strndup(
			((t_ast_node *)node->children.ctx)[0].token.start,
		((t_ast_node *)node->children.ctx)[0].token.len
	));
}

/* Take the single expanded value string out of `args` (ownership transfer).
   The caller's expand_word_assign_ro is expected to have produced exactly
   one field (keep-as-one + no-glob).  The ft_assert(args->len == 1) fires
   in debug builds if that contract is violated. */
static char	*dup_value_from_args(t_vec *args)
{
	char	*val;

	if (!args->len)
		return (NULL);
	ft_assert(args->len == 1);
	val = ((char **)args->ctx)[0];
	if (val)
		return (val);
	return (ft_strdup(""));
}

/* Turn an already-word-expanded subscript string into a numeric index:
   the result is a bare arithmetic expression (variables and $((...)) are
   gone), so arith_expand handles it directly. Empty -> 0. */
static long	arr_sub_index(t_shell *state, const char *sub)
{
	char	*res;
	long	idx;

	if (!sub || !*sub)
		return (0);
	res = arith_expand(state, sub, (int)ft_strlen(sub));
	idx = 0;
	if (res)
		idx = ft_atoi(res);
	return (xfree(res), idx);
}

/* arr[expr]=v: the key still carries its "[expr]" suffix here. Evaluate
   the subscript arithmetically, rebuild the array value with that
   element set (scalars promote to a one-element array first), truncate
   the key to the bare name. A plain key passes through untouched. */
static void	subscript_assign(t_shell *state, t_env *ret)
{
	char	*br;
	char	*res;
	char	*nv;
	char	*old;
	long	idx;

	br = ft_strchr(ret->key, '[');
	if (!br || !ret->value)
		return ;
	*br = '\0';
	old = env_expand(state, ret->key);
	if (assoc_is(old))
	{
		res = expand_param_word(state, br + 1,
				(int)ft_strlen(br + 1) - 1, false);
		nv = assoc_with_set(old, res, (int)ft_strlen(res), ret->value);
		xfree(res);
		return (xfree(ret->value), (void)(ret->value = nv));
	}
	res = expand_param_word(state, br + 1, (int)ft_strlen(br + 1) - 1, false);
	idx = arr_sub_index(state, res);
	xfree(res);
	nv = arr_with_set(old, idx, ret->value);
	xfree(ret->value);
	ret->value = nv;
}

/* NAME+=value: the key still carries its trailing '+'. Prepend the
   variable's current value (string concatenation, bash scalar +=) and
   drop the '+'. A subscript key (a[i]+=) keeps its brackets for
   subscript_assign; only the plain-scalar case concatenates here. */
static void	scalar_append(t_shell *state, t_env *ret)
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
	ret->value = (res ? res : ft_strdup("0"));
}

/* Resolve declare -n / -i attributes on a scalar assignment. A nameref
   retargets the write to its target name (re-checked for int-ness);
   an integer attribute arithmetic-evaluates the RHS. Array-element and
   append+nameref combos fall through unchanged (v1). */
static void	apply_var_attrs(t_shell *state, t_env *ret)
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

/* Convert a VAR=value AST node into a t_env ready to be pushed into the
   environment.  The RHS (child [1]) is expanded with assignment semantics
   (keep-as-one, no glob, malloc allocator rather than slab) so the resulting
   value is safe to store long-term.  The opt_allexport flag pre-marks the
   env entry as exported if `set -a` is active. */
t_env	assignment_to_env(t_shell *state, t_ast_node *node)
{
	t_vec	args;
	t_env	ret;
	char	*v;

	ret = (t_env){.exported = state->opt_allexport};
	vec_init(&args);
	args.elem_size = sizeof(char *);
	ft_assert(node->children.len == 2);
	expand_word_assign_ro(state,
		&((t_ast_node *)node->children.ctx)[1], &args);
	ret.key = dup_key_from_node(node);
	if (args.len)
	{
		v = dup_value_from_args(&args);
		if (v)
			ret.value = v;
		else
			ret.value = ft_strdup("");
	}
	apply_var_attrs(state, &ret);
	scalar_append(state, &ret);
	subscript_assign(state, &ret);
	return (xfree(args.ctx), ret);
}
