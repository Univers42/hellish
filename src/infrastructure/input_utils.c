/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_utils.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:10 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 16:32:02 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "input_private.h"

void get_more_input_parser(t_shell *state,
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

