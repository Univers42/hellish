/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_utils.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* Box-drawing + arrow glyphs – UTF-8 bytes, each exactly 1 display column. */
#define G_TL  "\xe2\x95\xad"        /* ╭ */
#define G_TR  "\xe2\x95\xae"        /* ╮ */
#define G_BL  "\xe2\x95\xb0"        /* ╰ */
#define G_H   "\xe2\x94\x80"        /* ─ */
#define G_ARR "\xe2\x9d\xaf"        /* ❯ */

/* A muted, modern palette (256-colour) – grey frame, cyan/blue segments. */
#define C_BOX  "\033[38;5;240m"
#define C_USER "\033[1m\033[38;5;81m"
#define C_CONN "\033[38;5;243m"
#define C_CWD  "\033[1m\033[38;5;75m"
#define C_GIT  "\033[1m\033[38;5;114m"
#define C_VENV "\033[38;5;179m"
#define C_TIME "\033[38;5;240m"
#define C_OK   "\033[1m\033[38;5;76m"
#define C_ERR  "\033[1m\033[38;5;203m"
#define C_RST  "\033[0m"

/* A dim connective word ("in" / "on"), e.g.  user >in< ~/cwd >on< branch. */
static void	push_conn(t_string *ret, t_prompt *p, const char *word)
{
	vec_push_str(ret, " ");
	vec_push_ansi(ret, C_CONN);
	vec_push_str(ret, (char *)word);
	vec_push_ansi(ret, C_RST);
	vec_push_str(ret, " ");
	p->vis_w += 2 + (int)ft_strlen(word);
}

/* Collapse a leading $HOME into "~", in place, for a cleaner cwd segment. */
static void	abbreviate_home(char *cwd)
{
	char	*home;
	size_t	hl;

	home = getenv("HOME");
	if (!home || !*home)
		return ;
	hl = ft_strlen(home);
	if (ft_strncmp(cwd, home, hl) != 0 || (cwd[hl] && cwd[hl] != '/'))
		return ;
	ft_memmove(cwd + 1, cwd + hl, ft_strlen(cwd + hl) + 1);
	cwd[0] = '~';
}

void	prompt_user_and_cwd(t_string *ret, t_prompt *p)
{
	char	cwd[PATH_MAX + 1];

	p->user = getenv("USER");
	if (!p->user)
		p->user = "user";
	if (!getcwd(cwd, sizeof(cwd)))
		ft_strcpy(cwd, "~");
	abbreviate_home(cwd);
	p->cols = get_cols();
	p->short_cwd = shorten_path(cwd, ft_max(20, p->cols - 50));
	p->vis_w = 0;
	vec_push_ansi(ret, C_BOX);
	vec_push_str(ret, G_TL G_H " ");
	vec_push_ansi(ret, C_RST);
	p->vis_w += 3;
	vec_push_ansi(ret, C_USER);
	vec_push_str(ret, p->user);
	vec_push_ansi(ret, C_RST);
	p->vis_w += (int)ft_strlen(p->user);
	push_conn(ret, p, "in");
	vec_push_ansi(ret, C_CWD);
	vec_push_str(ret, p->short_cwd);
	vec_push_ansi(ret, C_RST);
	p->vis_w += (int)ft_strlen(p->short_cwd);
}

void	prompt_branch(t_string *ret, t_prompt *p)
{
	p->branch = NULL;
	p->branch_dirty = 0;
	get_git_info(&p->branch, &p->branch_dirty);
	if (!p->branch)
		return ;
	push_conn(ret, p, "on");
	vec_push_ansi(ret, C_GIT);
	vec_push_str(ret, p->branch);
	vec_push_ansi(ret, C_RST);
	p->vis_w += (int)ft_strlen(p->branch);
}

void	prompt_venv(t_string *ret, t_prompt *p)
{
	p->venv = get_venv_name();
	if (!p->venv)
		return ;
	vec_push_str(ret, " ");
	vec_push_ansi(ret, C_VENV);
	vec_push_str(ret, "(");
	vec_push_str(ret, p->venv);
	vec_push_str(ret, ")");
	vec_push_ansi(ret, C_RST);
	p->vis_w += (int)ft_strlen(p->venv) + 3;
}

static void	push_fill(t_string *ret, int count)
{
	vec_push_str(ret, " ");
	vec_push_ansi(ret, C_BOX);
	while (count-- > 0)
		vec_push_str(ret, G_H);
	vec_push_ansi(ret, C_RST);
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
	vec_push_ansi(ret, C_TIME);
	vec_push_str(ret, p->time_buf);
	vec_push_ansi(ret, C_RST);
	vec_push_str(ret, " ");
	vec_push_ansi(ret, C_BOX);
	vec_push_str(ret, G_H G_TR);
	vec_push_ansi(ret, C_RST);
	vec_push_str(ret, "\n");
	vec_push_ansi(ret, C_BOX);
	vec_push_str(ret, G_BL G_H);
	vec_push_ansi(ret, C_RST);
	if (p->exit_status == 0)
		vec_push_ansi(ret, C_OK);
	else
		vec_push_ansi(ret, C_ERR);
	vec_push_str(ret, G_ARR " ");
	vec_push_ansi(ret, C_RST);
}
