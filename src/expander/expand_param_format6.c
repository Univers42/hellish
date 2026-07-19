/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_format6.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/19 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/19 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sh_input.h"

void	exit_clean(t_shell *state, int code);

/* The fatal expansion-error status bash uses depends on how input arrived:
   a -c command string exits 127, a script or piped stdin exits 1 (verified
   against bash --posix in both modes), and an interactive shell keeps
   running with $? = 1.  Shared by the ${p?w} error path below. */
static int	pf_fatal_status(t_shell *state)
{
	if (state->metinp == INP_ARG)
		return (127);
	return (1);
}

/* ${p?w} / ${p:?w}: if p is unset (or null with the colon), print the word
   to stderr and abort a non-interactive shell with the bash-parity status;
   interactively just set $? and return empty so the user keeps typing.
   Callers pass val=NULL to force the error branch (the @ and * path
   decides set-ness itself). */
char	*pf_err_word(t_shell *state, char *val, t_pe_op o)
{
	int	st;

	if (val && (!o.colon || *val != '\0'))
		return (ft_strdup(val));
	ft_eprintf("%s: %.*s: %.*s\n", state->ctx, o.name_len, o.name,
		o.wlen, o.word);
	st = pf_fatal_status(state);
	state->last_cmd_st_exe = create_exec_state(st, false);
	set_cmd_status(state, state->last_cmd_st_exe);
	if (state->metinp != INP_RL)
		exit_clean(state, st);
	return (ft_strdup(""));
}

/* ${@=w} / ${*=w} when the assignment would actually run: bash refuses
   ("$@: cannot assign in this way") and exits status 1 even under -c. */
char	*pf_assign_err(t_shell *state, t_pe_op o)
{
	ft_eprintf("%s: $%c: cannot assign in this way\n",
		state->ctx, o.name[0]);
	state->last_cmd_st_exe = create_exec_state(1, false);
	set_cmd_status(state, state->last_cmd_st_exe);
	if (state->metinp != INP_RL)
		exit_clean(state, 1);
	return (ft_strdup(""));
}

/* Expand an operator word that sits inside double quotes.  Rather than
   duplicating the reparser's dq escape rules, wrap the raw word in a pair
   of double quotes and run it through the normal unquoted pipeline: the
   reparser then applies exactly the "..." semantics — \$ \" \\ \` stay
   active escapes, any other backslash is kept literally ("${u-\z}" prints
   \z), tilde stays literal, and embedded quotes behave as they do in bash
   ("${u-'a b'}" keeps the single quotes). */
char	*expand_param_word_dq(t_shell *state, const char *word, int wlen)
{
	char	*buf;
	char	*ret;

	buf = xmalloc((size_t)wlen + 3);
	buf[0] = '"';
	ft_memcpy(buf + 1, word, (size_t)wlen);
	buf[wlen + 1] = '"';
	buf[wlen + 2] = '\0';
	ret = expand_param_word(state, buf, wlen + 2, false);
	xfree(buf);
	return (ret);
}

/* Token-level entry for the ${p-w} operator family: unlike the generic
   expand_param_format path (arith, heredoc) this one knows the enclosing
   token type, so it threads the double-quote context into the word
   expansion and routes the @ and * aggregates to expand_positional_op: it
   may need to re-emit one field per positional.  Returns false when the
   token is not an operator form so expand_token falls through. */
bool	expand_op_token(t_shell *state, t_token *tt, bool split_ctx)
{
	t_pe_op	o;
	char	*fmt;

	if (!find_param_op(tt->start, tt->len, &o))
		return (false);
	o.dq = (tt->tt == TT_DQENVVAR);
	if (o.name_len == 1 && (o.name[0] == '@' || o.name[0] == '*'))
	{
		expand_positional_op(state, tt, o, split_ctx);
		return (true);
	}
	fmt = expand_param_op(state, o);
	tt->start = fmt;
	tt->len = (int)ft_strlen(fmt);
	tt->allocated = true;
	return (true);
}
