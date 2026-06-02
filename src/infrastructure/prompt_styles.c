/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_styles.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "prompt_styles.h"

/* Push a 256-colour foreground as a zero-width (readline-safe) sequence. */
void	push_fg(t_string *ret, int color)
{
	char	*s;

	s = ft_itoa(color);
	if (!s)
		return ;
	vec_push_char(ret, '\001');
	vec_push_str(ret, "\033[38;5;");
	vec_push_str(ret, s);
	vec_push_str(ret, "m");
	vec_push_char(ret, '\002');
	free(s);
}

/* The aurora palette flows cyan -> blue -> violet and back; shifting the index
   by the frame makes the colours appear to drift left each new prompt. */
int	aurora_hue(size_t frame, int i)
{
	static const int	pal[14] = {51, 45, 39, 75, 111, 147, 183,
		219, 183, 147, 111, 75, 39, 45};

	return (pal[((i + (int)frame) % 14 + 14) % 14]);
}

/* Gather the live prompt data once (fork-free git via get_git_info). */
void	gather_info(t_prompt *p, size_t frame)
{
	char	cwd[PATH_MAX + 1];
	char	*home;

	(void)frame;
	p->user = getenv("USER");
	if (!p->user)
		p->user = "user";
	if (!getcwd(cwd, sizeof(cwd)))
		ft_strcpy(cwd, "~");
	home = getenv("HOME");
	if (home && *home && ft_strncmp(cwd, home, ft_strlen(home)) == 0
		&& (!cwd[ft_strlen(home)] || cwd[ft_strlen(home)] == '/'))
	{
		ft_memmove(cwd + 1, cwd + ft_strlen(home),
			ft_strlen(cwd + ft_strlen(home)) + 1);
		cwd[0] = '~';
	}
	p->cols = get_cols();
	p->short_cwd = shorten_path(cwd, ft_max(16, p->cols - 44));
	p->branch = NULL;
	p->branch_dirty = 0;
	get_git_info(&p->branch, &p->branch_dirty);
	p->venv = get_venv_name();
	get_timebuf(p->time_buf, sizeof(p->time_buf));
}

/* Free what gather_info allocated. */
void	free_info(t_prompt *p)
{
	free(p->short_cwd);
	free(p->branch);
	free(p->venv);
}

/* Animated braille spinner: one of 10 frames, advancing per prompt. */
void	push_spinner(t_string *ret, size_t f)
{
	static const char	*frames[10] = {"\xe2\xa0\x8b", "\xe2\xa0\x99",
		"\xe2\xa0\xb9", "\xe2\xa0\xb8", "\xe2\xa0\xbc", "\xe2\xa0\xb4",
		"\xe2\xa0\xa6", "\xe2\xa0\xa7", "\xe2\xa0\x87", "\xe2\xa0\x8f"};

	vec_push_str(ret, (char *)frames[f % 10]);
}
