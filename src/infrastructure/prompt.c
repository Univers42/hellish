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

/* Build the whole prompt for a given animation frame and last exit status: the
   blinking mascot, the box (user/cwd/branch/venv), the clock, and the arrow.
   Called both by prompt_normal and, with an advancing frame, by the readline
   idle hook -- so the mascot keeps blinking even while a command is typed. */
void	render_prompt(t_string *ret, size_t frame, int status)
{
	t_prompt	p;
	int			mascot_w;

	p.exit_status = status;
	mascot_w = push_mascot(ret, frame, status);
	prompt_user_and_cwd(ret, &p);
	p.vis_w += mascot_w;
	prompt_branch(ret, &p);
	prompt_venv(ret, &p);
	prompt_time_and_pad(ret, &p);
	free(p.short_cwd);
	if (p.branch)
		free(p.branch);
	if (p.venv)
		free(p.venv);
}

t_string	prompt_normal(t_shell *state)
{
	t_string	ret;
	int			status;

	ensure_locale();
	vec_init(&ret);
	ret.elem_size = 1;
	status = state->last_cmd_st_exe.status;
	(*anim_frame())++;
	*anim_status() = status;
	render_prompt(&ret, *anim_frame(), status);
	return (ret);
}
