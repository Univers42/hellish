/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   buffered_readline_utils.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:00 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

/* Re-sync the has_line flag from the raw buffer state. The buffer acts as a
   ring: data already consumed lives below cursor, unread data above it. We
   never remove consumed bytes until buff_readline_reset; until then cursor is
   our read pointer and has_line just says "is there more to deliver?". */
void	buff_readline_update(t_rl *l)
{
	if (!l->buff.ctx || l->buff.len == 0)
	{
		l->cursor = 0;
		l->has_line = false;
		return ;
	}
	if (l->cursor > l->buff.len)
		l->cursor = l->buff.len;
	l->has_line = l->cursor != l->buff.len;
}

/* Slide the unconsumed tail to the front of the buffer (compaction). The
   no_compact flag suppresses this when the caller wants to keep the raw bytes
   in place — used after a heredoc extraction where the stripped text is kept
   alive as a separate allocation. */
void	buff_readline_reset(t_rl *l)
{
	if (l->no_compact)
		return (buff_readline_update(l));
	if (l->buff.len > l->cursor)
		ft_memmove((char *)l->buff.ctx, (char *)l->buff.ctx + l->cursor,
			l->buff.len - l->cursor);
	else if (l->buff.len > 0)
		ft_memmove((char *)l->buff.ctx, (char *)l->buff.ctx, l->buff.len);
	l->buff.len -= l->cursor;
	if (l->buff.ctx)
		((char *)l->buff.ctx)[l->buff.len] = 0;
	l->cursor = 0;
	buff_readline_update(l);
}

/* Zero-initialise all fields: cursor=0, has_line=false, buff empty. */
void	buff_readline_init(t_rl *ret)
{
	*ret = (t_rl){};
}

/* Update the error-message context string to reflect the current line number.
   Bash writes "script: line N:" in error output — we mirror that here.
   Only runs when should_update_ctx is set (non-interactive script mode). */
void	update_ctx(t_shell *state)
{
	if (!state->rl.should_update_ctx)
		return ;
	xfree(state->ctx);
	state->ctx = (char *)ft_asprintf("%s: line %i",
			state->dft_ctx, state->rl.line);
}

/* Non-TTY path: read raw bytes straight from fd 0 (piped script). We stop on
   the first newline so the REPL loop still gets one command at a time, or on
   EOF to signal end-of-script. SIGINT during the read sets status=2 so the
   caller can cancel the current command without exiting the shell. */
int	get_more_input_notty(t_shell *state)
{
	char	buff[4096 * 2];
	int		ret;
	int		status;

	status = 1;
	set_unwind_sig_norestart();
	state->rl.buff.elem_size = 1;
	while (1)
	{
		ret = read(0, buff, sizeof(buff));
		if (ret < 0 && errno == EINTR)
			status = 2;
		if (ret == 0)
			state->rl.has_finished = true;
		if (ret == 0)
			vec_push_str(&state->rl.buff, "\n");
		if (ret <= 0)
			break ;
		status = 0;
		vec_push_nstr(&state->rl.buff, buff, ret);
		if (ft_strnchr(buff, '\n', ret))
			break ;
	}
	return (set_unwind_sig(),
		buff_readline_update(&state->rl), status);
}
