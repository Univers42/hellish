/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_more_tokens.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:32 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:32 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"
#include "sh_error.h"
#include "helpers.h"
#include "redir.h"
#include "job_control.h"

bool	heredoc_incomplete(const char *str);

/* Report EOF or truncated input with the right error to match bash's wording.
   If the tokenizer was waiting for a closing `, do:  or fi when EOF arrived,
   it says "looking for `X'"; otherwise "unexpected end of file". The "exit"
   echoed for interactive mode mirrors what bash prints when you hit ^D.

   Ctrl-D is a way OUT of the shell, so it owes the same duty the `exit`
   builtin does: refuse once while a stopped job is still on the table.
   This path used to set should_exit directly, so exit_stopped_guard
   protected only the builtin -- and every report of a terminal left
   unusable by `top &` came in through Ctrl-D (issue #58). An EMPTY buffer
   is the real "I am done" gesture; a partial line is a syntax error and
   still falls through below.

   When the guard refuses, the Ctrl-D has been SPENT on the warning and the
   shell has to go back to reading -- so rl_eof_rearm() clears the latch.
   state->rl.has_finished is sticky by design (buff_readline returns EOF
   immediately forever once it is set, which is what makes a closed stdin
   terminate a script promptly). Leaving it set here meant the warning
   printed and the very next REPL turn saw EOF again with exit_warned
   already true, so the shell left anyway -- warned and gone in one
   keypress, which is not what "ask me twice" means. */
static void	handle_eof_or_error(t_shell *state, t_deque_tok *tt)
{
	if (!state->input.len && state->metinp == INP_RL)
	{
		ft_eprintf("exit\n");
		if (!rl_eof_exit_ok(state))
			return ;
		state->should_exit = true;
		return ;
	}
	if (tt->looking_for && state->input.len)
		ft_eprintf("%s: unexpected EOF while looking for "
			"matching `%c'\n",
			state->ctx, tt->looking_for);
	else if (state->input.len)
	{
		ft_eprintf("%s: syntax error: unexpected end of file\n",
			state->ctx);
		zsh_brace_hint(state, tt);
	}
	if (state->metinp == INP_RL)
		ft_eprintf("exit\n");
	if (!state->last_cmd_st_exe.status && state->input.len)
		set_cmd_status(state, (t_execution_state){.status = SYNTAX_ERR});
	state->should_exit = true;
}

/* After each successful readline, tokenize what we have so far and check
   whether the tokenizer needs more input (prompt != NULL means "keep going").
   extend_bs strips trailing backslash-newlines first, then the alias
   scanner derives the expanded copy the lexer actually parses — aliases
   must be spliced before tokens exist for keywords in alias bodies to
   work (state->input keeps the original bytes for history/errors).
   When heredoc bodies have accumulated inline (multi-line compound or a
   batched cycle), the completeness check MUST tokenize the body-stripped
   text: body content is arbitrary (C code full of apostrophes in an
   autoconf configure) and would invert the tokenizer's quote state,
   making it declare an unterminated construct "complete" — the bug that
   broke ./configure at scale. The tokens then point into the transient
   stripped copy, which is safe: try_parse_tokens re-tokenizes via
   extract_input_heredocs (into the long-lived hd_stripped) before any
   token text is read. */
/* One-shot token-deque capacity for a big batch tokenize: real scripts
   average ~2.5 bytes per token, so bytes/2 slots can never regrow — the
   ring's doubling path memcpy'd ~60MB of t_token on a 50k-line batch.
   Only kicks in past 4KB, so interactive lines and eval bodies keep the
   default geometry. Replacing the buffer wholesale is safe exactly when
   the pending tokens are about to be discarded anyway: the tokenizer
   re-clears and re-tokenizes the whole buffer on every call, and tokens
   are value-copied out of the ring during parsing, never referenced. */
static void	deque_presize(t_deque *d, size_t bytes)
{
	size_t	want;

	if (bytes < 4096)
		return ;
	want = bytes / 2 + 64;
	if (d->cap >= want)
		return ;
	xfree(d->buff);
	d->buff = xmalloc(want * d->elem_size);
	d->cap = want;
	d->start = 0;
	d->end = 0;
	d->len = 0;
}

static void	update_prompt(t_shell *state, char **prompt, t_deque_tok *tt)
{
	char	*stripped;
	char	*bodies;

	extend_bs(state);
	alias_scan_update(state);
	deque_presize(&tt->deqtok, state->alias_exp.len);
	stripped = NULL;
	if (state->alias_exp.ctx && ft_strnstr((char *)state->alias_exp.ctx,
			"<<", state->alias_exp.len)
		&& split_heredocs((char *)state->alias_exp.ctx, &stripped, &bodies))
	{
		*prompt = tokenizer(stripped, tt);
		xfree(stripped);
		xfree(bodies);
	}
	else
		*prompt = tokenizer((char *)state->alias_exp.ctx, tt);
	if (*prompt)
		*prompt = ft_strdup(*prompt);
	else if (state->gathering_compound && state->input.ctx
		&& heredoc_incomplete((char *)state->input.ctx))
		*prompt = ft_strdup("> ");
}

/* Keep reading lines until the tokenizer stops asking for more (prompt becomes
   NULL). Handles EOF and interrupt in the loop: EOF always breaks and returns
   the error code; interrupt passes straight up to the caller who knows how to
   clear the in-progress command and repaint the prompt. */
int	get_more_tokens(t_shell *state, char **prompt, t_deque_tok *tt)
{
	int	stat;

	while (*prompt)
	{
		stat = readline_cmd(state, prompt);
		if (stat == 1)
		{
			if (try_replay_exact(state))
				return (5);
			handle_eof_or_error(state, tt);
		}
		if (stat)
			return (stat);
		update_prompt(state, prompt, tt);
	}
	return (0);
}
