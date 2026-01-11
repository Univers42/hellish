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

#include "shell.h"
#include "input.h"
#include <stdbool.h>
# include "lexer.h"
# include "parser.h"
# include "prompt.h"

// Add prototypes for static inline functions used from input.h
bool ends_with_bs_nl(t_string s);
bool is_empty_token_list(t_deque_tt *tokens);

bool	ends_with_bs_nl(t_string s)
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

bool	is_empty_token_list(t_deque_tt *tokens)
{
	/* tokens->deqtok is the internal t_deque */
	if (tokens->deqtok.len < 2)
		return (true);
	if (tokens->deqtok.len == 2
		&& ((t_token *)deque_idx(&tokens->deqtok, 0))->tt == TT_NEWLINE)
		return (true);
	return (false);
}

int	readline_cmd(t_shell *state, char **prompt)
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

void	extend_bs(t_shell *state)
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

void	get_more_tokens(t_shell *state, char **prompt, t_deque_tt *tt)
{
	int		stat;

	while (*prompt)
	{
		stat = readline_cmd(state, prompt);
		if (stat == 1)
		{
			if (tt->looking_for && state->input.len)
				ft_eprintf("%s: unexpected EOF while looking for "
					"matching `%c'\n", state->context, tt->looking_for);
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
			return ;
		*prompt = (extend_bs(state), tokenizer((char *)state->input.ctx, tt));
		if (*prompt)
			*prompt = ft_strdup(*prompt);
	}
}

bool	try_parse_tokens(t_shell *state, t_parser *parser,
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

static void	get_more_input_parser(t_shell *state,
			t_parser *parser, char **prompt, t_deque_tt *tt)
{
	while (parser->res == RES_MoreInput || parser->res == RES_Init)
	{
		get_more_tokens(state, prompt, tt);
		if (g_should_unwind)
			set_cmd_status(state, (t_exe_res){.status = CANCELED, .c_c = true});
		if (state->should_exit || g_should_unwind)
			break ;
		if (!try_parse_tokens(state, parser, tt, prompt))
			break ;
	}
}

void	parse_and_execute_input(t_shell *state)
{
	t_deque_tt	tt;
	char		*prompt;
	t_parser	parser;

	parser = (t_parser){.res = RES_Init};
	/* Initialize parse_stack as a vec of int (legacy vec_int) */
	vec_init(&parser.parse_stack);
	parser.parse_stack.elem_size = sizeof(int);

	prompt = (char *)prompt_normal(state).ctx;
	/* initialize token deque wrapper */
	tt = (t_deque_tt){0};
	deque_init(&tt.deqtok, 100, sizeof(t_token));
	tt.looking_for = 0;
	get_more_input_parser(state, &parser, &prompt, &tt);
	if (parser.res == RES_OK)
	{
		execute_top_level(state);
		free_ast(&state->tree);
	}
	if (g_should_unwind)
		set_cmd_status(state, (t_exe_res){.status = CANCELED, .c_c = true});
	manage_history(state);
	free (parser.parse_stack.ctx);
	parser.parse_stack = (t_vec_int){};
	free(tt.deqtok.buff);
	state->should_exit |= (g_should_unwind
			&& state->input_method != INP_READLINE)
		|| state->readline_buff.has_finished;
}
