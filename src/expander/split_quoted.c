/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   split_quoted.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 17:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 17:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* Append one element of a QUOTED list expansion -- "$@", "${a[@]}",
   "${!a[@]}" -- to the field being built.
     The token is TT_DQENVVAR, which is what tells the glob stage the text
   was quoted (star_expandable). These fields used to be built with
   push_new_env_child, whose TT_ENVVAR means UNQUOTED, so every element was
   pathname-expanded: `f() { rm -f -- "$@"; }; f '*'` removed every file in
   the directory. */
void	push_new_dq_child(t_ast_node *curr_node, char *new_start)
{
	t_ast_node	tmp;

	tmp = new_env_node(new_start);
	tmp.token.tt = TT_DQENVVAR;
	vec_push(&curr_node->children, &tmp);
}

/* Quoted "${!name[@]}": one verbatim field per index or key. The unquoted
   form is emit_keys_fields; `name` still has its leading '!'. */
void	emit_keys_at(t_shell *state, const char *name,
			t_ast_node *curr_node, t_vec_nd *ret)
{
	char		*val;
	const char	*cur;
	const char	*v;
	long		idx;
	int			nth[2];

	val = env_expand(state, (char *)name + 1);
	if (!val)
		return ;
	if (assoc_is(val))
		return (emit_assoc_fields(val, curr_node, ret,
				EMIT_KEYS | EMIT_QUOTED));
	cur = "";
	if (arr_is(val))
		cur = val + 1;
	nth[0] = 0;
	while (arr_next(&cur, &idx, &v, &nth[1]))
	{
		if (nth[0]++ > 0)
			push_and_reinit_curr_node(ret, curr_node);
		push_new_dq_child(curr_node, ft_itoa((int)idx));
	}
}

/* A used operator word, as pf_op_word_segments parked it: a quoted segment
   joins the field being built verbatim, an unquoted one is IFS-split into
   it like any unquoted expansion, its fields glob-eligible. So
   ${u:-$P"$P"} with P='a b c' is three fields, a / b / ca b c, as in
   bash. */
void	emit_op_segments(t_shell *state, const char *enc,
			t_ast_node *curr_node, t_vec_nd *ret)
{
	const char	*cur;
	const char	*v;
	long		idx;
	int			vl;
	char		*s;

	cur = enc + 1;
	while (arr_next(&cur, &idx, &v, &vl))
	{
		s = ft_strndup(v + 1, (size_t)(vl - 1));
		if (v[0] == 'q')
			push_new_dq_child(curr_node, s);
		else
		{
			split_value(state, s, curr_node, ret);
			xfree(s);
		}
	}
}
