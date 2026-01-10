/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:16:41 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:16:41 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INPUT_H
# define INPUT_H

# include "common.h"

enum e_input_method
{
	INP_READLINE,
	INP_FILE,
	INP_ARG,
	INP_STDIN_NOTTY,
};

// 1 - EOF
//
// 2 - C-c
static inline int	readline_cmd(t_state *state, char **prompt)
{
	int			stat;

	stat = buff_readline(state, &state->input, *prompt);
	free(*prompt);
	*prompt = 0;
	if (stat == 0)
	{
		return (1);
	}
	if (stat == 2)
	{
		if (state->input_method != INP_READLINE)
			state->should_exit = true;
		return (2);
	}
	return (0);
}

static inline bool	ends_with_bs_nl(t_string s)
{
	size_t	i;
	bool	unterminated;

	if (s.len == 0)
		return (false);
	i = s.len;
	unterminated = false;
	if (((char *)s.ctx)[--i] != '\n')
		return (false);
	while (i > 0)
	{
		i--;
		if (((char *)s.ctx)[i] == '\\')
			unterminated = !unterminated;
		else
			break ;
	}
	return (unterminated);
}

static inline void	extend_bs(t_state *state)
{
	char	*prompt;

	while (ends_with_bs_nl(state->input))
	{
		vec_pop(&state->input);
		vec_pop(&state->input);
		prompt = ft_strdup("> ");
		if (readline_cmd(state, &prompt))
			return ;
	}
}

static inline bool	is_empty_token_list(t_deque_tt *tokens)
{
	/* tokens->deqtok is the internal ft_deque */
	if (tokens->deqtok.len < 2)
		return (true);
	if (tokens->deqtok.len == 2
		&& ((t_token *)ft_deque_idx(&tokens->deqtok, 0))->tt == TT_NEWLINE)
		return (true);
	return (false);
}

static inline bool	try_parse_tokens(t_state *state, t_parser *parser,
	t_deque_tt *tt, char **prompt)
{
	if (is_empty_token_list(tt))
	{
		buff_readline_reset(&state->readline_buff);
		return (false);
	}
	parser->parse_stack.len = 0;
	state->tree = parse_tokens(state, parser, tt);
#if TRACE_DEBUG
	ft_eprintf("%s: debug: parser.res=%d\n", state->context, (int)parser->res);
#endif
	if (parser->res == RES_OK)
		return (true);
	else if (parser->res == RES_MoreInput)
	{
		*prompt = (char *)prompt_more_input(parser).ctx;
	}
	else if (parser->res == RES_FatalError)
		set_cmd_status(state, (t_exe_res){.status = SYNTAX_ERR});
	free_ast(&state->tree);
	return (true);
}

bool	ends_with_bs_nl(t_string s);
void	extend_bs(t_state *state);
void	get_more_tokens(t_state *state, char **prompt, t_deque_tt *tt);

# endif