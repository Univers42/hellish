/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_more_tokens.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:32 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:32 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"

// Add prototypes for static inline functions used from input.h
bool ends_with_bs_nl(t_string s);
bool is_empty_token_list(t_deque_tt *tokens);


int get_more_tokens(t_shell *state, char **prompt, t_deque_tt *tt)
{
	int stat;

	while (*prompt)
	{
		stat = readline_cmd(state, prompt);
		if (stat == 1)
		{
			if (tt->looking_for && state->input.len)
				ft_eprintf("%s: unexpected EOF while looking for "
						   "matching `%c'\n",
						   state->context, tt->looking_for);
			else if (state->input.len)
				ft_eprintf("%s: syntax error: unexpected end of file\n",
						   state->context);
			if (state->input_method == INP_READLINE)
				ft_eprintf("exit\n");
			if (!state->last_cmd_status_res.status && state->input.len)
				set_cmd_status(state, (t_exe_res){.status = SYNTAX_ERR});
			state->should_exit = true;
		}
		if (stat)
			return stat;
		*prompt = (extend_bs(state), tokenizer((char *)state->input.ctx, tt));
		if (*prompt)
			*prompt = ft_strdup(*prompt);
	}
	return 0;
}

