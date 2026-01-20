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

t_string	prompt_more_input(t_parser *parser)
{
	t_string	ret;
	t_tt		curr;
	size_t		i;

	i = -1;
	vec_init(&ret);
	ret.elem_size = 1;
	while (++i < parser->parse_stack.len)
	{
		curr = *(int *)vec_idx(&parser->parse_stack, i++);
		if (curr == TT_BRACE_LEFT)
			vec_push_str(&ret, "subsh");
		else if (curr == TT_PIPE)
			vec_push_str(&ret, "pipe");
		else if (curr == TT_AND)
			vec_push_str(&ret, "cmdand");
		else if (curr == TT_OR)
			vec_push_str(&ret, "cmdor");
		else
			continue ;
		vec_push_str(&ret, " ");
	}
	if (ret.len > 0)
		((char *)ret.ctx)[ret.len - 1] = '>';
	return (vec_push_str(&ret, " "), ret);
}

t_string	prompt_normal(void)
{
	t_string	ret;
	t_prompt	p;

	ensure_locale();
	vec_init(&ret);
	ret.elem_size = 1;
	prompt_user_and_cwd(&ret, &p);
	prompt_branch(&ret, &p);
	prompt_venv(&ret, &p);
	prompt_branch_marker(&ret, &p);
	prompt_time_and_pad(&ret, &p);
	free(p.short_cwd);
	if (p.branch)
		free(p.branch);
	if (p.venv)
		free(p.venv);
	return (ret);
}
