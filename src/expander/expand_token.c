/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_token.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:30:31 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:30:31 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "parena.h"

char	*expand_param_format(t_shell *state, const char *s, int slen, bool dq);
void	nounset_abort(t_shell *state, const char *name, int len);

static bool	handle_empty_token(t_token *curr_tt)
{
	if (curr_tt->len != 0)
		return (false);
	if (curr_tt->start && (*curr_tt->start == '\''
			|| *curr_tt->start == '"'))
	{
		curr_tt->start = "";
		curr_tt->len = 0;
		curr_tt->allocated = false;
		return (true);
	}
	curr_tt->start = "$";
	curr_tt->len = 1;
	curr_tt->allocated = false;
	return (true);
}

static void	expand_simple_var(t_shell *state, t_token *curr_tt)
{
	char	*temp;

	temp = env_expand_n(state, curr_tt->start, curr_tt->len);
	if (!temp && state->opt_nounset)
		nounset_abort(state, curr_tt->start, curr_tt->len);
	if (temp && arr_is(temp))
	{
		curr_tt->start = arr_get_idx(temp, 0);
		if (!curr_tt->start)
			curr_tt->start = ft_strdup("");
		curr_tt->len = ft_strlen(curr_tt->start);
		curr_tt->allocated = true;
		return ((void)parena_note_attach());
	}
	curr_tt->start = temp;
	if (curr_tt->start)
		curr_tt->len = ft_strlen(temp);
	else
		curr_tt->len = 0;
	curr_tt->allocated = false;
}

/* $@ / $* handling at expansion time. In a split context the field
   structure must survive to the splitter, so the token is DEFERRED:
   start is pointed at a pos_mark() sentinel the splitter recognises by
   POINTER identity (an expanded variable whose value happens to be "@"
   can never alias it):
     "$@"      -> one field per positional, never re-split
     $@ and $* -> one field per positional, each IFS-split
   Only "$*" joins here (single field, IFS[0] separator), and everything
   joins in a no-split context (assignments, redirect targets). */
static bool	expand_positional(t_shell *state, t_token *curr_tt, bool split_ctx)
{
	char	*temp;

	if (curr_tt->len != 1)
		return (false);
	if (curr_tt->start[0] != '@' && curr_tt->start[0] != '*')
		return (false);
	if (split_ctx && curr_tt->tt == TT_DQENVVAR
		&& curr_tt->start[0] == '@')
		return (curr_tt->start = (char *)pos_mark('@'), true);
	if (split_ctx && curr_tt->tt == TT_ENVVAR)
		return (curr_tt->start = (char *)pos_mark(curr_tt->start[0]), true);
	temp = join_positionals(state);
	curr_tt->start = temp;
	curr_tt->len = (int)ft_strlen(temp);
	curr_tt->allocated = true;
	parena_note_attach();
	return (true);
}

/* The array-flavoured ${...} shapes, tried in the exact dispatch order the
   inline chain used: positional slices, ${#a[@]}, element operators, the
   extended forms, then plain subscripts.  Split out of expand_token only to
   stay inside the norm line budget. */
static bool	try_array_forms(t_shell *state, t_token *curr_tt, bool split_ctx)
{
	if (expand_zsh_flags(state, curr_tt, split_ctx))
		return (true);
	if (zsh_param_token(state, curr_tt))
		return (true);
	if (zsh_hash_token(state, curr_tt))
		return (true);
	if (expand_pos_slice(state, curr_tt))
		return (true);
	if (expand_array_op(state, curr_tt))
		return (true);
	if (expand_array_elem_op(state, curr_tt))
		return (true);
	if (expand_array_ext(state, curr_tt, split_ctx))
		return (true);
	return (expand_array_token(state, curr_tt, split_ctx));
}

/* Expand a single $-token (TT_ENVVAR or TT_DQENVVAR) in place.  Priority:
     1. $@ / $* in a split context (emit_positional_at or join_positionals)
     2. Empty-token edge cases (bare $ or $'')
     3. ${p-w} operator family (expand_op_token — token-level so it can
        thread the double-quote context and the @ and * aggregate names)
     4. Other ${...} formats (expand_param_format handles #, %, /, :off)
     5. Plain $name lookup (expand_simple_var, honours set -u / nounset)
   The result is written back into curr_tt->start in-place; allocated flag
   tracks ownership so free_token_res can release it safely. */
void	expand_token(t_shell *state, t_token *curr_tt, bool split_ctx)
{
	char	*fmt;

	if (expand_positional(state, curr_tt, split_ctx))
		return ;
	if (handle_empty_token(curr_tt))
		return ;
	if (try_array_forms(state, curr_tt, split_ctx))
		return ;
	if (expand_op_token(state, curr_tt, split_ctx))
		return ;
	fmt = expand_param_format(state, curr_tt->start, curr_tt->len, false);
	if (fmt)
	{
		curr_tt->start = fmt;
		curr_tt->len = ft_strlen(fmt);
		curr_tt->allocated = true;
		parena_note_attach();
		return ;
	}
	expand_simple_var(state, curr_tt);
}
