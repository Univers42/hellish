/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_utils.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 18:24:38 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

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
	vec_push_ansi(ret, "\033[90m");
	vec_push_str(ret, "╭─");
	vec_push_ansi(ret, "\033[0m");
	vec_push_str(ret, " ");
	vec_push_ansi(ret, "\033[38;5;208m");
	vec_push_str(ret, p->user);
	vec_push_ansi(ret, "\033[0m");
	vec_push_str(ret, " ");
	vec_push_ansi(ret, "\033[90m");
	vec_push_str(ret, "─");
	vec_push_ansi(ret, "\033[0m");
	vec_push_str(ret, " ");
	vec_push_ansi(ret, "\033[36m");
	vec_push_str(ret, p->short_cwd);
	vec_push_ansi(ret, "\033[0m");
}

void	prompt_branch(t_string *ret, t_prompt *p)
{
	p->branch = NULL;
	p->branch_dirty = 0;
	get_git_info(&p->branch, &p->branch_dirty);
	if (p->branch)
	{
		vec_push_str(ret, " ");
		vec_push_ansi(ret, "\033[90m");
		vec_push_str(ret, "─");
		vec_push_ansi(ret, "\033[0m");
		vec_push_str(ret, " ");
		if (p->branch_dirty)
			vec_push_ansi(ret, "\033[35m");
		else
			vec_push_ansi(ret, "\033[37m");
		vec_push_str(ret, p->branch);
		vec_push_ansi(ret, "\033[0m");
	}
}

void	prompt_venv(t_string *ret, t_prompt *p)
{
	p->venv = get_venv_name();
	if (p->venv)
	{
		vec_push_str(ret, " ");
		vec_push_ansi(ret, "\033[34m");
		vec_push_str(ret, "(");
		vec_push_str(ret, p->venv);
		vec_push_str(ret, ")");
		vec_push_ansi(ret, "\033[0m");
	}
}

void	prompt_time_and_pad(t_string *ret, t_prompt *p)
{
	get_timebuf(p->time_buf, sizeof(p->time_buf));
	p->left_width = measure_width("╭─") + 1 + ft_strlen(p->user)
		+ 1 + measure_width("─") + 1 + ft_strlen(p->short_cwd);
	if (p->branch)
		p->left_width += 1 + measure_width("─") + 1 + ft_strlen(p->branch);
	if (p->venv)
		p->left_width += 2 + ft_strlen(p->venv) + 1;
	p->time_width = measure_width("⏳") + ft_strlen(p->time_buf);
	p->pad = p->cols - p->left_width - p->time_width;
	if (p->pad < 1)
		p->pad = 1;
	while (p->pad-- > 0)
		vec_push_char(ret, ' ');
	vec_push_ansi(ret, "\033[2m");
	vec_push_str(ret, "⏳");
	vec_push_str(ret, p->time_buf);
	vec_push_ansi(ret, "\033[0m");
	vec_push_str(ret, "\n");
	vec_push_str(ret, "$ ");
}

void	prompt_branch_marker(t_string *ret, t_prompt *p)
{
	if (p->branch_dirty)
		vec_push_str(ret, "*");
}
