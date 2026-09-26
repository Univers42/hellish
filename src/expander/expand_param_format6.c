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
   interactively set $?, and discard the rest of the line -- the command
   this word belongs to does not run, and neither does what follows its
   `;`, which is bash's DISCARD.
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
	state->discard_line = true;
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
	state->discard_line = true;
	return (ft_strdup(""));
}

/* Token-level entry for the ${p-w} operator family: unlike the generic
** expand_param_format path (arith, heredoc) this one knows the enclosing
** token type, so it threads the double-quote context into the word
** expansion and routes the @ and * aggregates to expand_positional_op: it
** may need to re-emit one field per positional.  Returns false when the
** token is not an operator form so expand_token falls through.
**
** FIELDS. What comes out unquoted is split and globbed, as bash does it:
** the variable's own value (${p:-w} with p set, ${p:=w}, ${p:?w}) exactly
** like $p, and a USED word of - or + part by part -- its unquoted text and
** expansions split and globbed, its quoted parts kept whole -- through
** pf_op_word_segments. `${u:-"c d"}` is one field, `${u:-$P}` and
** `${u:-*.c}` are not.
**   The whole token used to be retyped to TT_DQWORD (one field, no glob)
** whenever the word's TEXT held no unquoted blank. That test looked at the
** word even when the word was not used, so P='a b c'; ${P:-x} was one
** field; and it could not see inside an expansion, so ${u:-$P} was never
** split and ${u:-a*} never globbed.
**   An unquoted result that comes out empty is no field at all, which
** git-completion's `git ${a:+"${a[@]}"} ${d:+--git-dir="$d"}` relies on;
** `${x:-""}` is still one empty field, because its word quoted it.
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
	if (used && split_ctx && (pf_op_word_at_fields(state, tt, o.word, o.wlen)
			|| (tt->tt == TT_ENVVAR && (o.opc == '-' || o.opc == '+')
				&& pf_op_word_segments(state, tt, o))))
		return (true);
	fmt = expand_param_op(state, o);
	tt->start = fmt;
	tt->len = (int)ft_strlen(fmt);
	tt->allocated = true;
	parena_note_attach();
	return (true);
}
