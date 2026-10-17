/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_rprompt2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

/* The pieces rl_rprompt.c paints with: where the text ends, whether the
   right prompt fits, and the one-write frame. */

/* The last column the line can occupy, or -1 when the cursor may not be
   on the prompt's row (a search prompt owns it, the line has a newline).
   Per byte: a tab takes at most 8 columns, a control character is drawn
   as ^X (2), a byte a non-UTF-8 locale prints as \ooo takes 4, and every
   other byte at most 1 -- a UTF-8 character never takes more columns than
   it has bytes. */
int	rl_text_end(t_rl *l)
{
	unsigned char	c;
	int				end;
	int				i;

	if (RL_ISSTATE(RL_STATE_ISEARCH) || RL_ISSTATE(RL_STATE_NSEARCH))
		return (-1);
	end = l->rp_prompt_w;
	i = 0;
	while (rl_line_buffer && i < rl_end)
	{
		c = (unsigned char)rl_line_buffer[i++];
		if (c == '\n')
			return (-1);
		if (c == '\t')
			end += 8;
		else if (c < ' ' || c == 0x7f)
			end += 2;
		else if (c >= 0x80 && MB_CUR_MAX == 1)
			end += 4;
		else
			end += 1;
	}
	return (end);
}

/* Where to clear from: the upper bound above, unless a ghost is on screen.
   Clearing a few cells too far right is free; but a ghost left in them
   would stay, so with one painted the exact end of the (plain) line is
   used -- the same width the ghost was placed with. */
int	rl_text_col(t_rl *l)
{
	int	end;

	end = rl_text_end(l);
	if (end >= 0 && l->sg_painted && rl_suggest_plain(rl_line_buffer))
		end = rl_prompt_cols(l) + visible_width_cstr(rl_line_buffer);
	return (end);
}

/* The clock fits when one blank separates it from what ends at `end`. */
bool	rl_rp_fits(t_rl *l, int cols, int end)
{
	return (l->rp_w > 0 && cols > l->rp_w + 1 && end + 1 < cols - l->rp_w);
}

/* Append CSI <n> G (cursor to column n). False if it could not be built,
   and then nothing may be cleared: the clear would land at the cursor. */
bool	rl_push_col(t_string *f, int n)
{
	char	*s;

	s = ft_itoa(n);
	if (!s)
		return (false);
	vec_push_str(f, "\033[");
	vec_push_str(f, s);
	vec_push_str(f, "G");
	xfree(s);
	return (true);
}

/* One frame, one write: save; then either the ghost, dim, from the cursor
   (which readline left at the end of the text) or a jump to just past the
   text; clear to the end of the row; the clock if it is shown; restore.
   The ghost is cleared past as well, so a longer one that was there before
   cannot leave a tail. */
void	rl_frame(t_rl *l, t_frame *f)
{
	t_string	b;

	vec_init(&b);
	b.elem_size = 1;
	vec_push_str(&b, "\0337");
	if (f->gn)
	{
		vec_push_str(&b, "\033[2m");
		vec_push_nstr(&b, f->ghost, f->gn);
		vec_push_str(&b, "\033[0m");
	}
	else if (!rl_push_col(&b, f->end + 1))
		return (xfree(b.ctx));
	vec_push_str(&b, "\033[K");
	if (f->show && !rl_push_col(&b, f->cols - l->rp_w + 1))
		return (xfree(b.ctx));
	if (f->show)
		vec_push_str(&b, (char *)l->rp_txt.ctx);
	vec_push_str(&b, "\0338");
	rl_out_write(b.ctx, b.len);
	xfree(b.ctx);
	l->rp_painted = f->show;
	l->sg_painted = f->gn > 0;
	l->sg_col = f->end;
}
