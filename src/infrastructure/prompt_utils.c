/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_utils.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* Box-drawing + arrow glyphs – UTF-8 bytes, each exactly 1 display column. */
#define G_TL  "\xe2\x95\xad"        /* ╭ */
#define G_H   "\xe2\x94\x80"        /* ─ */

#define C_RST  "\033[0m"

/* A dim connective word ("in" / "on"), e.g.  user >in< ~/cwd >on< branch. */
static void	push_conn(t_string *ret, t_prompt *p, const char *word)
{
	vec_push_str(ret, " ");
	vec_push_ansi(ret, pal(PAL_CONN));
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

/* Emit the top row of the two-row prompt box: "╭─ user in ~/cwd". Also sets
   up p->cols and p->vis_w (visible width so far) for the right-side padding
   calculation done later in prompt_time_and_pad. The user and cwd segments
   themselves live in prompt_utils5.c (SSH host badge, dim-parent path). */
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
	vec_push_ansi(ret, pal(PAL_BOX));
	vec_push_str(ret, G_TL G_H " ");
	vec_push_ansi(ret, C_RST);
	p->vis_w += 3;
	push_user_seg(ret, p);
	push_conn(ret, p, "in");
	push_cwd_seg(ret, p);
}

/* Append " on <branch>" when inside a git repo, plus an amber "*" when the
   working tree has tracked modifications (git_dirty_cached — an async,
   TTL-throttled `git status -uno`: the render never waits for the scan, so
   on a huge repo the star may arrive a render late rather than the prompt
   arriving seconds late). Branch names count display columns, not bytes. */
void	prompt_branch(t_string *ret, t_prompt *p)
{
	p->branch = NULL;
	p->branch_dirty = 0;
	get_git_info(&p->branch, &p->branch_dirty);
	if (!p->branch)
		return ;
	push_conn(ret, p, "on");
	vec_push_ansi(ret, pal(PAL_GIT));
	vec_push_str(ret, p->branch);
	vec_push_ansi(ret, C_RST);
	p->vis_w += measure_width(p->branch);
	if (p->branch_dirty)
	{
		vec_push_ansi(ret, pal(PAL_DIRTY));
		vec_push_str(ret, "*");
		vec_push_ansi(ret, C_RST);
		p->vis_w += 1;
	}
}

/* Append "(venv-name)" if a Python virtual environment or conda env is active.
   CONDA_DEFAULT_ENV wins over VIRTUAL_ENV so conda users get the right name.
   For VIRTUAL_ENV we strip the leading path and show just the directory name
   (the env's basename), matching how fish and starship do it. */
void	prompt_venv(t_string *ret, t_prompt *p)
{
	p->venv = get_venv_name();
	if (!p->venv)
		return ;
	vec_push_str(ret, " ");
	vec_push_ansi(ret, pal(PAL_VENV));
	vec_push_str(ret, "(");
	vec_push_str(ret, p->venv);
	vec_push_str(ret, ")");
	vec_push_ansi(ret, C_RST);
	p->vis_w += measure_width(p->venv) + 3;
}
