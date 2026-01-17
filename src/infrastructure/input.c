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
#include "lexer.h"
#include "parser.h"
#include "prompt.h"
#include "ast.h"

// Add prototypes for static inline functions used from input.h
bool ends_with_bs_nl(t_string s);
bool is_empty_token_list(t_deque_tt *tokens);

bool ends_with_bs_nl(t_string s)
{
	size_t i;
	bool unterminated;

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
			break;
	}
	return (unterminated);
}

bool is_empty_token_list(t_deque_tt *tokens)
{
	/* tokens->deqtok is the internal t_deque */
	if (tokens->deqtok.len < 2)
		return (true);
	if (tokens->deqtok.len == 2 && ((t_token *)deque_idx(&tokens->deqtok, 0))->tt == TT_NEWLINE)
		return (true);
	return (false);
}

int readline_cmd(t_shell *state, char **prompt)
{
	int stat;

	/* defensive: ensure readline buffer vector initialized */
	if (!state->readline_buff.buff.ctx)
	{
		vec_init(&state->readline_buff.buff);
		state->readline_buff.buff.elem_size = 1;
	}
	/* defensive: ensure input buffer vector initialized */
	if (!state->input.ctx)
	{
		vec_init(&state->input);
		state->input.elem_size = 1;
	}
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

void extend_bs(t_shell *state)
{
	char *prompt;

	while (ends_with_bs_nl(state->input))
	{
		vec_pop(&state->input);
		vec_pop(&state->input);
		prompt = ft_strdup("> ");
		if (readline_cmd(state, &prompt))
			return;
	}
}

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

bool try_parse_tokens(t_shell *state, t_parser *parser,
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
	{
		/* Only set SYNTAX_ERR if status wasn't already set by the parser
		   (e.g., arithmetic errors set status to 1) */
		if (state->last_cmd_status_res.status == 0)
			set_cmd_status(state, (t_exe_res){.status = SYNTAX_ERR});
	}
	free_ast(&state->tree);
	return (true);
}

static void get_more_input_parser(t_shell *state,
								  t_parser *parser, char **prompt, t_deque_tt *tt)
{
	/* Debug parser mode: drive lexer+parser, print AST (textual), do NOT execute */
	if (state->option_flags & OPT_FLAG_DEBUG_PARSER)
	{
		while (parser->res == RES_MoreInput || parser->res == RES_Init)
		{
			/* ensure status reset for new input cycle */
			set_cmd_status(state, res_status(0));
			int s = get_more_tokens(state, prompt, tt);
			if (s == 1)
			{
				/* EOF */
				state->should_exit = true;
				break;
			}
			if (s == 2)
			{
				/* interrupted (ctrl-c) -> reset buffers similar to lexer-debug path */
				buff_readline_reset(&state->readline_buff);
				if (tt->deqtok.buff)
					deque_clear(&tt->deqtok, NULL);
				tt->looking_for = 0;
				if (state->input.ctx)
					state->input.len = 0;
				set_cmd_status(state, (t_exe_res){.status = 130, .c_c = true});
				if (*prompt)
					free(*prompt);
				{
					t_string _p = prompt_normal(state);
					*prompt = ft_strdup(_p.ctx);
					free(_p.ctx);
				}
				buff_readline_update(&state->readline_buff);
				/* attempt to immediately read next input (like lexer debug) */
				s = get_more_tokens(state, prompt, tt);
				if (s == 1)
				{
					state->should_exit = true;
					break;
				}
				if (s == 2)
				{
					/* another interrupt: continue waiting */
					continue;
				}
			}
			/* normal input read: ensure status reset immediately */
			set_cmd_status(state, res_status(0));
			if (get_g_sig()->should_unwind)
				set_cmd_status(state, (t_exe_res){.status = CANCELED, .c_c = true});
			if (state->should_exit || get_g_sig()->should_unwind)
				break;
			/* empty token list -> nothing to do for parser debug */
			if (is_empty_token_list(tt))
			{
				buff_readline_reset(&state->readline_buff);
				continue;
			}
			/* Parse tokens and print AST (textual) but don't execute */
			parser->parse_stack.len = 0;
			{
				t_ast_node parsed = parse_tokens(state, parser, tt);
#if TRACE_DEBUG
				ft_eprintf("%s: debug: parser.res=%d\n", state->context, (int)parser->res);
#endif
				/* Always print the AST, even if incomplete (RES_MoreInput), so user can see partial parse */
				if (parser->res == RES_OK || parser->res == RES_MoreInput)
				{
					print_ast_dot(state, parsed);
				}
				if (parser->res == RES_OK || parser->res == RES_FatalError || parser->res == RES_MoreInput)
					free_ast(&parsed);
				if (parser->res == RES_FatalError)
					set_cmd_status(state, (t_exe_res){.status = SYNTAX_ERR});
			}
			/* cleanup token deque and input similar to lexer debug path */
			if (tt->deqtok.buff)
				deque_clear(&tt->deqtok, NULL);
			tt->looking_for = 0;
			manage_history(state);
			buff_readline_reset(&state->readline_buff);
			buff_readline_update(&state->readline_buff);
			state->input.len = 0;
			if (*prompt)
				free(*prompt);
			{
				t_string _p = prompt_normal(state);
				*prompt = ft_strdup(_p.ctx);
				free(_p.ctx);
			}
			/* ensure parser won't execute the parsed AST in the caller */
			parser->res = RES_Init;
		}
		return;
	}

	/* If caller requested lexer-only debug, drive the readline/tokenizer
	   infrastructure but do not parse/execute: print tokens and continue. */
	if (state->option_flags & OPT_FLAG_DEBUG_LEXER)
	{
		while (parser->res == RES_MoreInput || parser->res == RES_Init)
		{
			/* ensure status reset for new input cycle */
			set_cmd_status(state, res_status(0));
			int s = get_more_tokens(state, prompt, tt);
			if (s == 1)
			{
				/* EOF */
				state->should_exit = true;
				break;
			}
			if (s == 2)
			{
				/* interrupted (ctrl-c) */
				buff_readline_reset(&state->readline_buff);
				if (tt->deqtok.buff)
					deque_clear(&tt->deqtok, NULL);
				tt->looking_for = 0;
				if (state->input.ctx)
					state->input.len = 0;
				set_cmd_status(state, (t_exe_res){.status = 130, .c_c = true});
				if (*prompt)
					free(*prompt);
				{
					t_string _p = prompt_normal(state);
					*prompt = ft_strdup(_p.ctx);
					free(_p.ctx);
				}
				buff_readline_update(&state->readline_buff);
				/* attempt to immediately read the next input so it is not lost */
				s = get_more_tokens(state, prompt, tt);
				if (s == 1)
				{
					state->should_exit = true;
					break;
				}
				if (s == 2)
				{
					/* another interrupt: continue waiting */
					continue;
				}
				/* if we received a normal input immediately after Ctrl-C, process it now */
				if (!is_empty_token_list(tt))
				{
					/* print tokens for debugging */
					print_tokens(tt);
					/* set successful status */
					set_cmd_status(state, res_status(0));
					/* clear token deque */
					deque_clear(&tt->deqtok, NULL);
					tt->looking_for = 0;
					/* record history and update readline state */
					manage_history(state);
					buff_readline_reset(&state->readline_buff);
					buff_readline_update(&state->readline_buff);
					/* mark input buffer empty but keep memory */
					state->input.len = 0;
					/* refresh prompt */
					if (*prompt)
						free(*prompt);
					{
						t_string _p = prompt_normal(state);
						*prompt = ft_strdup(_p.ctx);
						free(_p.ctx);
					}
					/* continue loop */
					continue;
				}
				/* reset status for the newly read input so it's not affected by previous Ctrl-C */
				set_cmd_status(state, res_status(0));
				/* fall through to normal processing for the newly read input */
			}
			/* normal input read: ensure status reset immediately */
		 set_cmd_status(state, res_status(0));
			if (get_g_sig()->should_unwind)
				set_cmd_status(state, (t_exe_res){.status = CANCELED, .c_c = true});
			if (state->should_exit || get_g_sig()->should_unwind)
				break;
			if (is_empty_token_list(tt))
			{
				buff_readline_reset(&state->readline_buff);
				continue;
			}
			/* print tokens for debugging */
			print_tokens(tt);
			/* set successful status for this debug run */
			set_cmd_status(state, res_status(0));
			/* clear token deque */
			deque_clear(&tt->deqtok, NULL);
			tt->looking_for = 0;
			/* record history then reset/readline update */
			manage_history(state);
			buff_readline_reset(&state->readline_buff);
			buff_readline_update(&state->readline_buff);
			/* mark input buffer empty but keep memory for history */
			state->input.len = 0;
			/* refresh prompt */
			if (*prompt)
				free(*prompt);
			{
				t_string _p = prompt_normal(state);
				*prompt = ft_strdup(_p.ctx);
				free(_p.ctx);
			}
		}
		return;
	}

	/* default behavior: parse tokens and proceed to execution when ready */
	while (parser->res == RES_MoreInput || parser->res == RES_Init)
	{
		int s = get_more_tokens(state, prompt, tt);
		if (s == 1)
		{
			state->should_exit = true;
			break;
		}
		if (s == 2)
		{
			set_cmd_status(state, (t_exe_res){.status = CANCELED, .c_c = true});
			continue;
		}
		if (get_g_sig()->should_unwind)
			set_cmd_status(state, (t_exe_res){.status = CANCELED, .c_c = true});
		if (state->should_exit || get_g_sig()->should_unwind)
			break;
		if (!try_parse_tokens(state, parser, tt, prompt))
			break;
	}
}

void parse_and_execute_input(t_shell *state)
{
	t_deque_tt tt;
	char *prompt;
	t_parser parser;

	parser = (t_parser){.res = RES_Init};
	/* Initialize parse_stack as a vec of int (legacy vec_int) */
	vec_init(&parser.parse_stack);
	parser.parse_stack.elem_size = sizeof(int);

	{
		t_string p = prompt_normal(state);
		prompt = ft_strdup(p.ctx);
		free(p.ctx);
	}
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
	if (get_g_sig()->should_unwind)
		set_cmd_status(state, (t_exe_res){.status = CANCELED, .c_c = true});
	manage_history(state);
	/* cleanup resources allocated in this function */
	if (parser.parse_stack.ctx)
		free(parser.parse_stack.ctx);
	parser.parse_stack = (t_vec_int){};
	if (prompt)
		free(prompt);
	if (tt.deqtok.buff)
		free(tt.deqtok.buff);
	state->should_exit |= (get_g_sig()->should_unwind && state->input_method != INP_READLINE) || state->readline_buff.has_finished;
}
