/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:34:22 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:34:22 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include <stdio.h>

#include <readline/readline.h>
# include "parser.h"

t_string	prompt_more_input(t_parser *parser)
{
	t_string	ret;
	t_tt		curr;
	size_t		i;

	i = 0;
	vec_init(&ret);
	while (i < parser->parse_stack.len)
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
	((char *)ret.ctx)[ret.len - 1] = '>';
	vec_push_str(&ret, " ");
	return (ret);
}

t_string	prompt_normal(t_shell *state)
{
	t_string	ret;

	vec_init(&ret);
	if (state->last_cmd_status_res.status == 0)
		vec_push_str(&ret, ANSI_GREEN);
	else
	{
		vec_push_str(&ret, ANSI_RED);
		vec_push_str(&ret, state->last_cmd_status_s);
		vec_push_str(&ret, " ");
	}
	vec_push_str(&ret, PROMPT);
	vec_push_str(&ret, ANSI_RESET);
	vec_push_str(&ret, RL_SPACER_1);
	vec_push_str(&ret, RL_SPACER_1);
	return (ret);
}
