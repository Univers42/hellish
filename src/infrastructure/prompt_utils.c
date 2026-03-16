/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_utils.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 12:43:35 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* Box-drawing glyphs – UTF-8 bytes, each exactly 1 display column wide.  */
/* Resolved at compile time: no runtime encoding / decoding is needed.     */
#define G_TL  "\xe2\x95\xad"   /* ╭ */
#define G_TR  "\xe2\x95\xae"   /* ╮ */
#define G_BL  "\xe2\x95\xb0"   /* ╰ */
#define G_H   "\xe2\x94\x80"   /* ─ */

static void	push_sep(t_string *ret, t_prompt *p)
{
	vec_push_str(ret, " ");
	vec_push_ansi(ret, "\033[38;5;55m");
	vec_push_str(ret, G_H G_H);
	vec_push_ansi(ret, "\033[0m");
	vec_push_str(ret, " ");
	p->vis_w += 4;
}

void	prompt_user_and_cwd(t_string *ret, t_prompt *p)
{
	char	cwd[PATH_MAX + 1];

	p->user = getenv("USER");
	if (!p->user)
		p->user = "user";
	if (!getcwd(cwd, sizeof(cwd)))
		ft_strcpy(cwd, "~");
	p->cols = get_cols();
	p->short_cwd = shorten_path(cwd, ft_max(20, p->cols - 50));
	p->vis_w = 0;
	vec_push_ansi(ret, "\033[38;5;55m");
	vec_push_str(ret, G_TL G_H);
	vec_push_ansi(ret, "\033[0m");
	vec_push_str(ret, " ");
	p->vis_w += 3;
	vec_push_ansi(ret, "\033[1m\033[38;5;141m");
	vec_push_str(ret, p->user);
	vec_push_ansi(ret, "\033[0m");
	p->vis_w += (int)ft_strlen(p->user);
	push_sep(ret, p);
	vec_push_ansi(ret, "\033[1m\033[38;5;183m");
	vec_push_str(ret, p->short_cwd);
	vec_push_ansi(ret, "\033[0m");
	p->vis_w += (int)ft_strlen(p->short_cwd);
}

void	prompt_branch(t_string *ret, t_prompt *p)
{
	p->branch = NULL;
	p->branch_dirty = 0;
	get_git_info(&p->branch, &p->branch_dirty);
	if (!p->branch)
		return ;
	push_sep(ret, p);
	if (p->branch_dirty)
		vec_push_ansi(ret, "\033[1m\033[38;5;204m");
	else
		vec_push_ansi(ret, "\033[1m\033[38;5;114m");
	vec_push_str(ret, p->branch);
	vec_push_ansi(ret, "\033[0m");
	p->vis_w += (int)ft_strlen(p->branch);
}

void	prompt_venv(t_string *ret, t_prompt *p)
{
	p->venv = get_venv_name();
	if (!p->venv)
		return ;
	push_sep(ret, p);
	vec_push_ansi(ret, "\033[38;5;75m");
	vec_push_str(ret, "(");
	vec_push_str(ret, p->venv);
	vec_push_str(ret, ")");
	vec_push_ansi(ret, "\033[0m");
	p->vis_w += (int)ft_strlen(p->venv) + 2;
}

void	prompt_branch_marker(t_string *ret, t_prompt *p)
{
	if (p->branch_dirty)
	{
		vec_push_ansi(ret, "\033[38;5;204m");
		vec_push_str(ret, "*");
		vec_push_ansi(ret, "\033[0m");
		p->vis_w += 1;
	}
}

static void	push_fill(t_string *ret, int count)
{
	vec_push_str(ret, " ");
	vec_push_ansi(ret, "\033[38;5;55m");
	while (count-- > 0)
		vec_push_char(ret, '-');
	vec_push_ansi(ret, "\033[0m");
	vec_push_str(ret, " ");
}

void	prompt_time_and_pad(t_string *ret, t_prompt *p)
{
	int	right_w;

	get_timebuf(p->time_buf, sizeof(p->time_buf));
	right_w = (int)ft_strlen(p->time_buf) + 3;
	p->pad = p->cols - p->vis_w - right_w;
	if (p->pad < 3)
		p->pad = 3;
	push_fill(ret, p->pad - 2);
	vec_push_ansi(ret, "\033[2m\033[38;5;245m");
	vec_push_str(ret, p->time_buf);
	vec_push_ansi(ret, "\033[0m");
	vec_push_str(ret, " ");
	vec_push_ansi(ret, "\033[38;5;55m");
	vec_push_str(ret, G_H G_TR);
	vec_push_ansi(ret, "\033[0m");
	vec_push_str(ret, "\n");
	vec_push_ansi(ret, "\033[38;5;55m");
	vec_push_str(ret, G_BL G_H);
	vec_push_ansi(ret, "\033[0m");
	if (p->branch_dirty)
		vec_push_ansi(ret, "\033[1m\033[38;5;204m");
	else
		vec_push_ansi(ret, "\033[1m\033[38;5;141m");
	vec_push_str(ret, "> ");
	vec_push_ansi(ret, "\033[0m");
}
