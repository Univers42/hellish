/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_repaint.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "zle.h"

/* Repainting the prompt while a line is being read.
**
** The prompt is rendered before the read starts, and what it shows can
** change during it: the background git scan (prompt_git3.c) finishes, or a
** widget moves the shell and calls `zle reset-prompt`. A scan used to be
** harvested by the NEXT render only, so a repository whose `git status`
** outlasted the short wait showed its dirty star one Enter late.
**
** Repainting is only done where it cannot land on the user's text: a PS1
** read in this process, no nested reader (search, completion, a numeric
** argument) owning the row, a single-row input line -- and, unless a widget
** asked for it, nothing typed yet. The rows above the input row are
** rewritten only when old and new prompt have as many and none of them
** wraps: the cursor is then provably count-newlines rows below them, the
** same assumption redraw_mascot makes. Prompts built by hook functions
** get HELLISH_PROMPT_REFRESH_FUNCS run first, so their PS1 can follow. */

static bool	rp_idle_row(t_shell *st, bool force)
{
	if (!st || st->rl.use_fork || !st->rl.ps1_read || !st->rl.shown_prompt)
		return (false);
	if (RL_ISSTATE(RL_STATE_ISEARCH | RL_STATE_NSEARCH
			| RL_STATE_COMPLETING | RL_STATE_NUMERICARG
			| RL_STATE_MOREINPUT | RL_STATE_VIMOTION | RL_STATE_MULTIKEY))
		return (false);
	if (rl_line_buffer && ft_strchr(rl_line_buffer, '\n'))
		return (false);
	return (force || rl_end == 0);
}

/* Every row of `s` but the last fits in `cols` columns, and the last one
   with the typed line after it. The count of rows goes to *rows. */
static bool	rp_fits(const char *s, int cols, int *rows)
{
	char	*row;
	char	*nl;
	int		w;

	*rows = 0;
	while (1)
	{
		nl = ft_strchr(s, '\n');
		if (nl)
			row = ft_strndup(s, (size_t)(nl - s));
		else
			row = ft_strdup(s);
		w = visible_width_cstr(row);
		xfree(row);
		if ((nl && w >= cols) || (!nl && w + rl_end + 1 >= cols))
			return (false);
		if (!nl)
			return (true);
		(*rows)++;
		s = nl + 1;
	}
}

/* Move to the first prompt row and write the new upper rows over the old,
   in one write, ending at column 0 of the input row, which is cleared for
   readline to redraw. Synchronized output (DEC 2026) keeps a terminal that
   knows it from showing the intermediate frame. */
static void	rp_upper_rows(const char *s, int rows)
{
	t_string	f;
	char		*n;

	vec_init(&f);
	f.elem_size = 1;
	vec_push_str(&f, "\033[?2026h\r");
	n = ft_itoa(rows);
	if (rows > 0 && n)
		(vec_push_str(&f, "\033["), vec_push_str(&f, n), vec_push_str(&f,
				"A"));
	xfree(n);
	while (rows-- > 0 && *s)
	{
		while (*s && *s != '\n')
		{
			if (*s != '\001' && *s != '\002')
				vec_push_char(&f, *s);
			s++;
		}
		vec_push_str(&f, "\033[K\r\n");
		s += (*s == '\n');
	}
	vec_push_str(&f, "\033[K");
	rl_out_write(f.ctx, f.len);
	xfree(f.ctx);
}

/* Put `p` on screen in place of the prompt shown now. */
static void	rp_swap(t_shell *st, t_string *p, int cols)
{
	int		old_rows;
	int		rows;
	char	*row;

	if (!rp_fits(st->rl.shown_prompt, cols, &old_rows)
		|| !rp_fits((char *)p->ctx, cols, &rows) || rows != old_rows)
		return (xfree(p->ctx));
	rp_upper_rows((char *)p->ctx, rows);
	row = ft_strrchr((char *)p->ctx, '\n');
	if (!row)
		row = (char *)p->ctx;
	else
		row++;
	rl_set_prompt(row);
	st->rl.rp_prompt_w = visible_width_cstr(row);
	rl_forced_update_display();
	rl_out_write("\033[?2026l", 8);
	xfree(st->rl.shown_prompt);
	st->rl.shown_prompt = (char *)p->ctx;
}

/* Re-render the prompt and show it if it changed; `force` is `zle
   reset-prompt`, which repaints whatever it renders to. */
void	rl_prompt_repaint(t_shell *st, bool force)
{
	t_rl_bracket	b;
	t_string		p;
	int				rows;
	int				cols;

	if (!rp_idle_row(st, force))
		return ;
	rl_shell_enter(st, &b);
	run_hook_funcs(st, "HELLISH_PROMPT_REFRESH_FUNCS", NULL);
	if (!rl_shell_leave(st, &b))
		return ;
	p = prompt_normal(st);
	if (!p.ctx)
		return ;
	rl_get_screen_size(&rows, &cols);
	if (!force && !st->rl.rp_w
		&& !ft_strcmp((char *)p.ctx, st->rl.shown_prompt))
		return (xfree(p.ctx));
	rp_swap(st, &p, cols);
}
