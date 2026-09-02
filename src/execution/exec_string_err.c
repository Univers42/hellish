/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_string_err.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "parser.h"
#include "sh_error.h"

/* Say something.  About a dozen parser productions set RES_ERR without a
   message of their own -- parse_for, parse_if, parse_while, parse_case,
   parse_array, parse_arith -- so `for x (a b)` and `source` of a file with
   an unterminated brace both failed with status 2 and complete silence.
   `reported` is set by unexpected() when it has already spoken; everything
   else lands here and gets bash's wording. */
void	report_parse_error(t_shell *state, t_parser *parser,
				t_deque_tok *tt)
{
	if (parser->res == RES_GETMOREINPUT)
	{
		ft_eprintf("%s: syntax error: unexpected end of file\n", state->ctx);
		zsh_brace_hint(state, tt);
	}
	else if (!parser->reported)
		unexpected(state, parser, (t_ast_node){0}, tt);
	set_cmd_status(state, res_status(2));
}
