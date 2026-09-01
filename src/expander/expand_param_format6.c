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
#include "parena.h"
#include "sh_input.h"

void	exit_clean(t_shell *state, int code);
int		shell_fatal_status(t_shell *state);
bool	opword_no_split(const char *w, int wlen);

/* The fatal expansion-error status bash uses depends on how input arrived:
   a -c command string exits 127, a script or piped stdin exits 1 (verified
   against bash --posix in both modes), and an interactive shell keeps
   running with $? = 1.  Shared by the ${p?w} error path below.

   The 127 belongs to the TOP-LEVEL shell exiting, not to a forked child:
   `bash --posix -c '( echo ${undef:?} )'` reports 1.  shell_fatal_status
   makes that call, shared with the -u and read-only paths. */
static int	pf_fatal_status(t_shell *state)
{
	return (shell_fatal_status(state));
}

/* ${p?w} / ${p:?w}: if p is unset (or null with the colon), print the word
   to stderr and abort a non-interactive shell with the bash-parity status;
   interactively just set $? and return empty so the user keeps typing.
   Callers pass val=NULL to force the error branch (the @ and * path
   decides set-ness itself).

   The word is an ordinary operator word, so it goes through
   expand_param_word() exactly like the - = + forms do: `${u:?$W}` reports
   W's value and `${u:?"a b"}` drops the quotes. Printing o.word raw meant
   neither happened. When the word is OMITTED entirely, POSIX leaves the
   text unspecified and bash supplies one; we use bash's two strings
   verbatim, which differ by the colon -- with it the parameter may be
   merely null, without it it is definitely unset (issue #15). */
char	*pf_err_word(t_shell *state, char *val, t_pe_op o)
{
	int		st;
	char	*msg;

	if (val && (!o.colon || *val != '\0'))
		return (ft_strdup(val));
	if (o.wlen > 0)
		msg = expand_param_word(state, o.word, o.wlen, o.dq);
	else if (o.colon)
		msg = ft_strdup("parameter null or not set");
	else
		msg = ft_strdup("parameter not set");
	ft_eprintf("%s: %.*s: %s\n", state->ctx, o.name_len, o.name, msg);
	xfree(msg);
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

/* Token-level entry for the ${p-w} operator family: unlike the generic
** expand_param_format path (arith, heredoc) this one knows the enclosing
** token type, so it threads the double-quote context into the word
** expansion and routes the @ and * aggregates to expand_positional_op: it
** may need to re-emit one field per positional.  Returns false when the
** token is not an operator form so expand_token falls through.
**
** THE EMPTY-RESULT GUARD. Retyping the token to TT_DQWORD says "this is one
** field, do not IFS-split it" -- right for `${x:-"c d"}`, and wrong when
** there is nothing there at all, because an unquoted expansion that comes
** out empty contributes NO field in POSIX. Without the guard:
**
**     a=(); d=; git ${a:+"${a[@]}"} ${d:+--git-dir="$d"} --version
**
** ran git with two extra empty arguments. git errors, prints nothing, and
** the completion function that called it returned an empty COMPREPLY --
** which reads as "no completions", not as "the shell built the wrong argv".
** That is git-completion's __git wrapper, verbatim.
**
** `used && o.wlen > 0` is the exception bash keeps: `${x:-""}` DID use its
** word, the word was a quoted empty string, and that is a real field.
*/
bool	expand_op_token(t_shell *state, t_token *tt, bool split_ctx)
{
	t_pe_op	o;
	char	*fmt;
	bool	used;

	if (!find_param_op(tt->start, tt->len, &o))
		return (false);
	o.dq = (tt->tt == TT_DQENVVAR);
	if (o.name_len == 1 && (o.name[0] == '@' || o.name[0] == '*'))
	{
		expand_positional_op(state, tt, o, split_ctx);
		return (true);
	}
	used = pf_op_word_used(pf_get_var_value(state, o.name, o.name_len), o);
	if (used && split_ctx
		&& pf_op_word_at_fields(state, tt, o.word, o.wlen))
		return (true);
	fmt = expand_param_op(state, o);
	tt->start = fmt;
	tt->len = (int)ft_strlen(fmt);
	tt->allocated = true;
	if (split_ctx && tt->tt == TT_ENVVAR && (tt->len > 0
			|| (used && o.wlen > 0)) && opword_no_split(o.word, o.wlen))
		tt->tt = TT_DQWORD;
	parena_note_attach();
	return (true);
}

/* Does the operator word contain NO unquoted IFS whitespace? Then its
   expansion is a single field even in a split context — `${x:-"c d"}`
   keeps "c d" whole because the space is inside quotes. Words WITH
   unquoted whitespace stay split-eligible (a mixed word like a "b c" d
   is a documented v1 divergence — flat splitting mislabels its middle
   field, but fully-quoted defaults, the common case, are now correct). */
bool	opword_no_split(const char *w, int wlen)
{
	int		i;
	char	q;

	i = 0;
	q = 0;
	while (i < wlen)
	{
		if (q && w[i] == q)
			q = 0;
		else if (!q && (w[i] == '"' || w[i] == '\''))
			q = w[i];
		else if (!q && (w[i] == ' ' || w[i] == '\t' || w[i] == '\n'))
			return (false);
		i++;
	}
	return (true);
}
