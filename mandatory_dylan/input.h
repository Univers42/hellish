/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 22:48:23 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 22:48:23 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INPUT_H
# define INPUT_H

typedef enum e_input_method
{
	INPUT_READLINE,
	INPUT_FILE,
	INPUT_ARG,
	INPUT_STDIN_NOTTY
}	t_input_method;

static inline t_string prompt_more_input(t_parser *parser)
{
	t_string	ret;
	t_tt		curr;
	size_t		i;

	i = 0;
	vec_init(&ret);
	while (i < parser->parse_stack.len)
	{
		curr = vec_idx(&parser->parse_stack, i++);
		if (curr == TT_BRACE_LEFT)
			vec_push(&ret, "subsh");
		else if (curr == TT_PIPE)
			vec_push(&ret, "pipe");
		else if (curr == TT_AND)
			vec_push(&ret, "cmdand");
		else if (curr == TT_OR)
			vec_push(&ret, "cmdor");
		else
			continue ;
		vec_push(&ret, " ");
	}
	ret.buff[ret.len - 1] = '>';
	vec_push(&ret, " ");
	return (ret);
}

static inline t_string	prompt_normal(t_state *state)
{
	t_string	ret;

	vec_init(&ret);
	if (state->last_cmd_status_res.status == 0)
		vec_push(&ret, ANSI_GREEN);
	else
	{
		vec_push(&ret, ANSI_RED);
		vec_push(&ret, state->last_cmd_status_s);
		vec_push(&ret, " ");
	}
	vec_push(&ret, PROMPT);
	vec_push(&ret, ANSI_RESET);
	vec_push(&ret, RL_SPACER_1);
	vec_push(&ret, RL_SPACER_1);
	return (ret);
}

#endif