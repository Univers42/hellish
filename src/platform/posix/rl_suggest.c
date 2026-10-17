/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_suggest.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

/* The inline suggestion: the newest history entry that starts with what
** is typed, shown dim after the cursor and never part of the line. This
** file decides WHAT may be suggested and WHEN; rl_suggest2.c turns it into
** the text and rl_rprompt.c paints it.
**
** Anything a terminal could act on is kept out of the ghost. An entry is
** written to the tty as it is, so one holding ESC (a colour, a title, a
** cursor move), a newline, a tab or any other control byte is skipped, as
** is a C1 control in its UTF-8 form: running `printf '\e[31m'` once must
** not repaint every later prompt. */

/* True when `s` holds nothing a terminal would interpret. 0x7f is DEL; the
   pair 0xC2 0x80..0x9F is U+0080..U+009F, which some terminals read as a
   control sequence introducer. */
static bool	sg_clean(const unsigned char *s)
{
	size_t	i;

	i = 0;
	while (s[i])
	{
		if (s[i] < 0x20 || s[i] == 0x7f)
			return (false);
		if (s[i] == 0xc2 && s[i + 1] >= 0x80 && s[i + 1] < 0xa0)
			return (false);
		i++;
	}
	return (true);
}

/* The newest of the last sg_max entries that starts with the first `n`
   bytes of the line, is longer than it and is clean. A bounded look keeps
   a huge history from slowing a keystroke down. */
static char	*sg_scan(t_shell *state, t_rl *l, size_t n)
{
	char	**a;
	size_t	i;
	size_t	seen;

	a = (char **)state->hist.hist_cmds.ctx;
	i = state->hist.hist_cmds.len;
	seen = 0;
	while (i > 0 && seen++ < (size_t)l->sg_max)
	{
		i--;
		if (a[i] && !ft_strncmp(a[i], rl_line_buffer, n)
			&& ft_strlen(a[i]) > n && sg_clean((unsigned char *)a[i]))
			return (a[i]);
	}
	return (NULL);
}

/* The entry to suggest for the line, or NULL.
**
** Typing extends the line, and an entry that matches the longer line
** matched the shorter one too -- so when the line still starts with the one
** the last scan was for, the answer is the same entry as long as it still
** matches (or "nothing" for good). That makes a key O(line) with no scan,
** and a paste of 4 KB one scan instead of 4096. Anything else (a
** Backspace, a recalled line) scans again. The history cannot change under
** it: it only grows between reads, and the cache is dropped per read. */
char	*rl_suggest_find(t_shell *state, t_rl *l)
{
	size_t	n;
	size_t	p;
	char	*e;

	n = (size_t)rl_end;
	p = 0;
	if (l->sg_known)
		p = ft_strlen(l->sg_prev);
	if (l->sg_known && p <= n && !ft_strncmp(rl_line_buffer, l->sg_prev, p)
		&& (!l->sg_ent || (ft_strlen(l->sg_ent) > n
				&& !ft_strncmp(l->sg_ent, rl_line_buffer, n))))
		return (l->sg_ent);
	xfree(l->sg_prev);
	xfree(l->sg_ent);
	e = sg_scan(state, l, n);
	l->sg_ent = NULL;
	if (e)
		l->sg_ent = ft_strdup(e);
	l->sg_prev = ft_strdup(rl_line_buffer);
	l->sg_known = true;
	return (l->sg_ent);
}

/* The line is plain text this painter can measure: no control byte (a ^X
   or a tab takes more columns than it has bytes), and in a single-byte
   locale no byte readline would draw as \ooo. */
bool	rl_suggest_plain(const char *s)
{
	size_t	i;

	i = 0;
	while (s[i])
	{
		if ((unsigned char)s[i] < 0x20 || (unsigned char)s[i] == 0x7f)
			return (false);
		if ((unsigned char)s[i] >= 0x80 && MB_CUR_MAX == 1)
			return (false);
		i++;
	}
	return (true);
}

/* Where a ghost may be shown at all: the cursor at the end of the PS1
** line, the editor in a plain insert state, not finishing, and the line --
** with the prompt -- narrower than the terminal so that neither it nor the
** ghost can wrap. The cursor is then on the prompt's row at column
** rp_prompt_w + width of the text, which is where the ghost goes.
**
** Not looked at: whether more input is already waiting. Asking costs a
** select(2) per key, which the per-key syscall budget does not allow; the
** cache above makes the extra frames cheap instead. */
bool	rl_suggest_gate(t_rl *l, int cols)
{
	if (rl_done || rl_point != rl_end || !l->ps1_read || !rl_line_buffer)
		return (false);
	if (RL_ISSTATE(RL_STATE_ISEARCH | RL_STATE_NSEARCH))
		return (false);
	if (rl_get_keymap() == vi_movement_keymap)
		return (false);
	if (!rl_suggest_plain(rl_line_buffer))
		return (false);
	return (rl_prompt_cols(l) + visible_width_cstr(rl_line_buffer) < cols - 1);
}
