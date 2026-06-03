/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:34:22 by marvin            #+#    #+#             */
/*   Updated: 2026/01/12 00:45:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "prompt_styles.h"

/* Build one of the experimental prompt styles into `ret`; returns true if a
   style was rendered (false => caller falls back to the classic box prompt). */
static bool	render_style(t_shell *state, t_string *ret, int style)
{
	size_t	f;
	int		st;

	f = state->prompt_frame;
	st = state->last_cmd_st_exe.status;
	if (style == STYLE_BREATHE)
		return (style_breathe(f, st, ret), true);
	if (style == STYLE_WAVE)
		return (style_wave(f, st, ret), true);
	if (style == STYLE_PULSE)
		return (style_pulse(f, st, ret), true);
	if (style == STYLE_POWERLINE)
		return (style_powerline(f, st, ret), true);
	if (style == STYLE_AURORA)
		return (style_aurora(f, st, ret), true);
	return (false);
}

static const char	*prompt_label(t_tt curr)
{
	if (curr == TT_BRACE_LEFT)
		return ("subsh");
	if (curr == TT_PIPE)
		return ("pipe");
	if (curr == TT_AND)
		return ("cmdand");
	if (curr == TT_OR)
		return ("cmdor");
	if (curr == TT_IF)
		return ("if");
	if (curr == TT_WHILE)
		return ("while");
	if (curr == TT_UNTIL)
		return ("until");
	if (curr == TT_FOR)
		return ("for");
	return (NULL);
}

t_string	prompt_more_input(t_parser *parser)
{
	t_string	ret;
	t_tt		curr;
	size_t		i;
	const char	*label;

	i = -1;
	vec_init(&ret);
	ret.elem_size = 1;
	while (++i < parser->parse_stack.len)
	{
		curr = *(int *)vec_idx(&parser->parse_stack, i++);
		label = prompt_label(curr);
		if (!label)
			continue ;
		vec_push_str(&ret, (char *)label);
		vec_push_str(&ret, " ");
	}
	if (ret.len > 0)
		((char *)ret.ctx)[ret.len - 1] = '>';
	return (vec_push_str(&ret, " "), ret);
}

t_string	prompt_normal(t_shell *state)
{
	t_string	ret;
	t_prompt	p;

	ensure_locale();
	vec_init(&ret);
	ret.elem_size = 1;
	state->prompt_frame++;
	*anim_frame() = state->prompt_frame;
	if (render_style(state, &ret, select_prompt_style()))
		return (ret);
	p.exit_status = state->last_cmd_st_exe.status;
	prompt_user_and_cwd(&ret, &p);
	prompt_branch(&ret, &p);
	prompt_venv(&ret, &p);
	prompt_time_and_pad(&ret, &p);
	free(p.short_cwd);
	if (p.branch)
		free(p.branch);
	if (p.venv)
		free(p.venv);
	return (ret);
}
