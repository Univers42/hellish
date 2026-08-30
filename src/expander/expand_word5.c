/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_word5.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:41:16 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:41:16 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

static char	*push_plain_literal(t_ast_node *src, t_vec *args)
{
	t_token	*t;
	char	*v;

	if (!word_is_plain_literal(src))
		return (NULL);
	t = &((t_ast_node *)src->children.ctx)[0].token;
	v = word_strndup(t->start, t->len);
	vec_push(args, &v);
	return (v);
}

static char	*fast_path_expand(t_shell *state, t_ast_node *src,
				t_vec *args, bool keep_as_one)
{
	char	*v;

	v = push_plain_literal(src, args);
	if (v)
		return (v);
	v = try_pure_arith(state, src);
	if (v)
		return (vec_push(args, &v), v);
	v = try_simple_envvar(state, src);
	if (v)
	{
		v = word_strndup(v, ft_strlen(v));
		return (vec_push(args, &v), v);
	}
	if (keep_as_one)
	{
		v = try_simple_concat(state, src);
		if (v)
			return (vec_push(args, &v), v);
	}
	return (NULL);
}

/* Non-destructive variant: expand `src` into `args` while leaving `src`
   intact.  This is required anywhere a node is reused across iterations
   (for-loop variable words, redirect targets in loops).  The fast paths in
   fast_path_expand are tried first; if they fire, no clone is needed at all.
   Only on the slow path do we pay for clone_ast. */
void	expand_word_ro(t_shell *state, t_ast_node *src,
			t_vec *args, bool keep_as_one)
{
	t_ast_node	scratch;

	if (fast_path_expand(state, src, args, keep_as_one))
		return ;
	scratch = clone_ast(src);
	expand_word(state, &scratch, args, keep_as_one);
}

/* Assignment-value expansion: like expand_word_ro but pathname expansion is
   suppressed (EW_NO_GLOB).  POSIX says VAR=value does not glob — only cmd
   args do.  Also disables the word slab (word_slab_push(0)) because the
   resulting string will be stored into the environment, which outlives the
   current command, so it must be a plain malloc'd pointer, not slab memory.
   The saved `o` is restored to avoid poisoning the caller's slab state. */
void	expand_word_assign_ro(t_shell *state, t_ast_node *src, t_vec *args)
{
	t_ast_node	scratch;
	int			o;

	o = word_slab_push(0);
	if (fast_path_expand(state, src, args, true))
	{
		word_slab_push(o);
		return ;
	}
	scratch = clone_ast(src);
	expand_word_glob_ctl(state, &scratch, args, EW_KEEP_AS_ONE | EW_NO_GLOB);
	word_slab_push(o);
}

/* One ELEMENT of an array literal: arr=(a $V *.md).
**
** Elements are not scalar assignments. bash word-splits and globs them --
** `arr=($V)` yields one element per field and `files=(*.c)` yields one per
** match -- and those are the two commonest array idioms there are. Routing
** them through expand_word_assign_ro (EW_KEEP_AS_ONE | EW_NO_GLOB) gave a
** one-element array holding the unsplit string, silently: no error, just a
** loop that ran once with the wrong value.
**
** So this passes flags 0, exactly like a command word. What it does NOT
** share with expand_word_ro is the slab: an array value is stored into the
** environment and outlives the command, so the slab is pushed off around the
** expansion (same reason and same idiom as the function above).
**
** The scalar path above is deliberately untouched. POSIX really does say
** VAR=value neither splits nor globs, so `x=*.md` must stay literal. */
void	expand_word_elem_ro(t_shell *state, t_ast_node *src, t_vec *args)
{
	t_ast_node	scratch;
	int			o;

	o = word_slab_push(0);
	scratch = clone_ast(src);
	expand_word_glob_ctl(state, &scratch, args, 0);
	word_slab_push(o);
}
