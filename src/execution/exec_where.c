/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_where.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 19:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/24 19:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Where the executing command is: the "FILE: line N" a runtime error
   starts with, and $LINENO inside a sourced file.

   ctx was only ever set by the READER, from the line it had read up to.
   Input arrives in batches -- a whole small script at once -- so by the
   time a command ran, the reader stood past the end of the batch, and
   every error in `printf 'true\n./nope\n' > t.sh; hellish t.sh` said
   "t.sh: line 3" where bash says line 2. Inside a sourced file it was
   worse: the error named the file that did the sourcing, at a line of
   its own, and $LINENO was that call site's too.

   Now each simple command, as it starts, moves ctx to where its first
   token sits -- in the sourced chunk it came from (err_pos) when there is
   one, else in the top-level cycle (tok_lineno). ctx is rebuilt only when
   that place changes. A token that lies in neither -- a function body,
   an eval string -- leaves ctx at the last place that could be named,
   the call site, which is also what $LINENO reports there. */

/* Newlines between two points of one text, signed. Resolving each line
   from the previous one keeps a file run top to bottom at one scan in
   total, and a loop at a rescan of its own body per iteration. */
int	nl_between(const char *from, const char *to)
{
	if (to >= from)
		return (nl_count(from, (size_t)(to - from)));
	return (-nl_count(to, (size_t)(from - to)));
}

/* The file line of `tok`, a token of the chunk whose first line is line0. */
int	srcpos_line(t_srcpos *p, int line0, const char *tok)
{
	if (p->memo)
		p->memo_line += nl_between(p->memo, tok);
	else
		p->memo_line = line0 + nl_count(p->base, (size_t)(tok - p->base));
	p->memo = tok;
	return (p->memo_line);
}

/* Called as each simple command starts, with its first token: in the
   sourced chunk, the file's line; in the top-level text, the script's
   (just the name at a prompt, which has no line to give); anywhere else
   -- a function body, an eval string -- ctx stays at the call site. */
void	where_follow(t_shell *state, const char *tok)
{
	int	line;

	if (srcpos_has(&state->err_pos, tok))
	{
		line = srcpos_line(&state->err_pos, state->err_line, tok);
		ctx_follow(state, state->err_src, line);
		return ;
	}
	line = cycle_lineno(state, tok);
	if (line < 0)
		return ;
	if (!state->rl.should_update_ctx)
		line = 0;
	ctx_follow(state, NULL, line);
}

/* A sourced file's chunk is about to run (text), or has run (NULL). eval
   and trap text is not a file: its chunks leave the frame alone. */
void	where_chunk(t_shell *state, const char *text)
{
	if (!state->err_src)
		return ;
	state->err_pos = (t_srcpos){0};
	if (!text)
		return ;
	state->err_pos.base = text;
	state->err_pos.len = ft_strlen(text);
}
