/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_string5.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/24 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "sh_input.h"

/* Where the chunker (exec_string3.c) cuts eval and source text. */

/* Which bytes end a span. A chunk's FIRST span (open=false) clips at
   every input hazard, so a top-level alias/shopt/source line always gets
   its own chunk and executes before later text lexes. GROWTH spans
   (open=true) are different: the parser has already demanded more, so we
   are inside an open construct -- and a hazard line INSIDE a construct
   cannot take effect before the whole construct executes in bash either
   (bash parses the compound first, then runs its body). Clipping there
   bought no correctness and cost a full re-splice/re-lex/re-parse of the
   accumulated chunk per interior hazard word: the 2244-line theme rc,
   one giant if/else with ~60 alias/source words inside, re-parsed ~80KB
   dozens of times and `source ~/.hellishrc` visibly lagged bash. Only
   `<<` still clips growth (heredoc consumption depends on delivery).
   The overshoot a growth span can add past the construct's close is the
   one divergence: a def-then-use pair landing in that tail lexes
   together -- accepted, and the statement replay still bounds errors. */
static bool	span_hazard(const char *s, size_t i, size_t n, bool open)
{
	if (!open)
		return (input_hazard_at(s, i, n));
	return (s[i] == '<' && i + 1 < n && s[i + 1] == '<');
}

/* Push clip past every line that a trailing backslash joins to the one
   before it. A chunk may not end between a `\` and its continuation: the
   tokenizer strips the join, so `echo a \` looks complete, the
   continuation becomes a separate statement, and `{ cmd \` inside an
   open group parses as an ERROR rather than as "needs more" -- the chunk
   was then replayed instead of grown. A growth span could stop there:
   there, a backslash-newline is no hazard, only `<<` is, so the retreat
   to a line start landed on the join (#139: nvm's `}' \<NL>| grep \<NL>
   | sort; } << EOF`). A `\` that only looks like a join (quoted, or in a
   comment) makes the span longer, which a growth span can always afford. */
static size_t	str_join_end(const char *s, size_t clip, size_t n)
{
	while (clip < n && clip > 1 && s[clip - 1] == '\n'
		&& s[clip - 2] == '\\')
	{
		while (clip < n && s[clip] != '\n')
			clip++;
		clip += (clip < n);
	}
	return (clip);
}

/* Longest hazard-free run of COMPLETE lines starting at off; if a hazard
   sits in the very first line, that one line. Either way the span is
   stretched to the end of its LOGICAL line, joins included. */
size_t	chunk_span(const char *s, size_t off, size_t n, bool open)
{
	size_t	clip;

	clip = off;
	while (clip < n && !span_hazard(s, clip, n, open))
		clip++;
	while (clip > off && s[clip - 1] != '\n')
		clip--;
	if (clip == off)
	{
		while (clip < n && s[clip] != '\n')
			clip++;
		clip += (clip < n);
	}
	return (str_join_end(s, clip, n) - off);
}
