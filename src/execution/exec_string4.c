/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_string4.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 17:25:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/01 17:25:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* The chunk driver half of the #105 fix; the chunk mechanics and the
   design rationale live in exec_string3.c. */

/* Execute the fully parsed statements of a chunk, in order, stopping on
   exit/return/errexit/unwind. Freeing is chunk_close's job so even the
   never-reached tail is reclaimed. */
static int	run_stmt_list(t_shell *state, t_chunkctx *c, bool *stop)
{
	size_t	i;
	int		status;

	status = 0;
	i = 0;
	while (i < c->asts.len && !*stop)
	{
		status = run_parsed(state, (t_ast_node *)c->asts.ctx + i);
		*stop = must_stop(state);
		i++;
	}
	return (status);
}

/* A chunk that would not parse is replayed statement-at-a-time from its
   own spliced text: the healthy prefix executes before the error is
   reported, which is bash's observable order (`eval 'echo ok; ('` prints
   ok first). Nothing ran before the replay -- the parse-all pass does
   not execute -- so no statement can run twice. run_one_stmt reports
   both flavors (hard error, unterminated-at-EOF). The chunk's error ends
   the whole exec_string, as it always has. */
static int	replay_chunk(t_shell *state, t_chunkctx *c, bool *stop)
{
	t_deque_tok	tt;
	int			status;

	tt = (t_deque_tok){0};
	deque_init(&tt.deqtok, 100, sizeof(t_ltoken));
	tokenizer(c->spliced, &tt);
	reclassify_keywords(&tt, zsh_mode(state));
	status = 0;
	skip_delimiters(&tt);
	while (!*stop && ((t_ltoken *)deque_peek(&tt.deqtok))->tt != TT_END)
		status = run_one_stmt(state, &tt, stop);
	xfree(tt.deqtok.buff);
	*stop = true;
	return (status);
}

/* One chunk, start to finish: open at the next hazard boundary, grow
   while the construct is incomplete and text remains (nothing executes
   during growth), then run it -- whole-chunk on success, replay on
   failure. Advances *off past the chunk either way. */
static int	run_chunk(t_shell *state, const char *s, size_t *off,
				bool *stop)
{
	t_chunkctx	c;
	size_t		n;
	int			status;

	n = ft_strlen(s);
	c = (t_chunkctx){0};
	c.start = *off;
	c.end = *off;
	chunk_grow(state, s, n, &c);
	while (c.parser.res == RES_GETMOREINPUT && c.end < n)
		chunk_grow(state, s, n, &c);
	if (c.parser.res == RES_OK)
		status = run_stmt_list(state, &c, stop);
	else
		status = replay_chunk(state, &c, stop);
	chunk_close(&c);
	*off = c.end;
	return (status);
}

/* Drive the whole string chunk by chunk. Clearing func_return at the end
   keeps a `return` inside eval from leaking into the caller's frame,
   same contract as the old single-pass loop. */
int	exec_chunks(t_shell *state, const char *str)
{
	size_t	off;
	size_t	n;
	int		status;
	bool	stop;

	off = 0;
	n = ft_strlen(str);
	status = 0;
	stop = false;
	while (off < n && !stop)
		status = run_chunk(state, str, &off, &stop);
	state->func_return = 0;
	return (status);
}
