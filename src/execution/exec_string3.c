/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_string3.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 17:20:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/01 17:20:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "lexer.h"
#include "parser.h"
#include "sh_alias.h"
#include "sh_input.h"

/* Chunked lex/parse/execute for eval and the dot builtin (issue #105).

   exec_string used to alias-splice and tokenize the WHOLE string before
   executing any of it, so a statement that changes how later text lexes
   -- `shopt -s extglob`, an alias definition, a nested source that does
   either -- had no effect on the rest of the same string. bash reads and
   runs incrementally, and real files depend on it: bash-completion arms
   extglob on line 47 and writes extglob case patterns ~1800 lines down,
   so `. bash_completion` (reached from ~/.bashrc at every login) died
   with "syntax error near unexpected token `('" on any Debian box.

   The string is now processed in chunks clipped at the same hazard bytes
   the input driver's batch reader uses (input_hazard_at: alias, source,
   shopt, `.`, heredocs, backslash-newline), on line boundaries. Each
   chunk is alias-spliced FRESH from the original text -- a chunk is
   spliced at most once, so an alias body can never be re-expanded -- and
   fully parsed before anything in it runs, exactly like one input-driver
   cycle. Statements after a hazard line therefore lex AFTER it executed.
   Same-line uses stay unspliced (the hazard line is its own chunk head),
   matching bash's aliases-apply-from-the-next-line rule.

   A chunk whose parse is incomplete grows by the next span and retries
   (nothing has executed yet, so the retry is free). A chunk that fails
   to parse is replayed statement-at-a-time so the healthy prefix still
   runs before the error is reported, which is bash's observable order.
   Known residual gap, shared with the batch reader: a lexer-state change
   hidden from the byte scan (a function body calling shopt, invoked in
   the same chunk that then uses extglob) still lexes ahead. */

/* Where each chunk ends is chunk_span's business (exec_string5.c). */

/* Parse every statement of the tokenized chunk WITHOUT executing any.
   Stops at the first non-OK result; only the tail statement can come
   back RES_GETMOREINPUT (an incomplete construct consumes the queue). */
static void	parse_all(t_shell *state, t_chunkctx *c)
{
	t_ast_node	ast;

	skip_delimiters(&c->tt);
	c->parser.quiet = true;
	while (((t_ltoken *)deque_peek(&c->tt.deqtok))->tt != TT_END)
	{
		c->parser.res = RES_OK;
		c->parser.reported = false;
		c->parser.parse_stack.len = 0;
		ast = parse_simple_list(state, &c->parser, &c->tt);
		vec_push(&c->asts, &ast);
		if (c->parser.res != RES_OK)
			return ;
		skip_delimiters(&c->tt);
	}
}

/* Splice, lex and fully parse one chunk of the original text. A lexically
   incomplete chunk (tokenizer still wants input: open quote, paren) is
   reported as RES_GETMOREINPUT so the caller grows it like any other
   incomplete construct. The tokenizer's prompt return is static. */
static void	chunk_open(t_shell *state, const char *at, size_t len,
				t_chunkctx *c)
{
	char	*more;

	c->chunk = ft_strndup((char *)at, len);
	c->spliced = alias_scan_line(&state->aliases, c->chunk);
	c->tt = (t_deque_tok){0};
	deque_init(&c->tt.deqtok, 100, sizeof(t_ltoken));
	c->parser = (t_parser){.res = RES_OK};
	vec_init(&c->parser.parse_stack);
	c->parser.parse_stack.elem_size = sizeof(int);
	vec_init(&c->asts);
	c->asts.elem_size = sizeof(t_ast_node);
	more = tokenizer(c->spliced, &c->tt);
	reclassify_keywords(&c->tt, zsh_mode(state));
	if (more)
		c->parser.res = RES_GETMOREINPUT;
	else
		parse_all(state, c);
	if (getenv("HELLISH_DBG_CHUNKS"))
		fprintf(stderr, "[chunk %zu-%zu res=%d]<<<%s>>>\n",
			c->start, c->end, c->parser.res, c->spliced);
}

/* Grow the chunk to the next hazard boundary and (re-)open it. Works for
   the FIRST open too: on a zeroed ctx with start == end, chunk_close only
   frees NULLs and the span extends from the chunk's start. Nothing of a
   smaller attempt has executed, so re-parsing from scratch is safe. */
void	chunk_grow(t_shell *state, const char *s, size_t n, t_chunkctx *c)
{
	chunk_close(c);
	c->end += chunk_span(s, c->end, n, c->end > c->start);
	chunk_open(state, s + c->start, c->end - c->start, c);
}
