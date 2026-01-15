/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/15 15:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith.h"
#include "libft.h"

static void	skip_whitespace(t_arith_lexer *lex)
{
	while (lex->pos < lex->len && 
		   (lex->input[lex->pos] == ' ' || lex->input[lex->pos] == '\t' ||
			lex->input[lex->pos] == '\n'))
		lex->pos++;
}

static bool	is_var_start(char c)
{
	return (ft_isalpha(c) || c == '_');
}

static bool	is_var_char(char c)
{
	return (ft_isalnum(c) || c == '_');
}

static void	lex_number(t_arith_lexer *lex)
{
	long long	val;
	int			base;
	int			start;

	val = 0;
	base = 10;
	start = lex->pos;
	/* Check for hex (0x) or octal (0) */
	if (lex->input[lex->pos] == '0' && lex->pos + 1 < lex->len)
	{
		if (lex->input[lex->pos + 1] == 'x' || lex->input[lex->pos + 1] == 'X')
		{
			base = 16;
			lex->pos += 2;
		}
		else if (ft_isdigit(lex->input[lex->pos + 1]))
		{
			base = 8;
			lex->pos++;
		}
	}
	while (lex->pos < lex->len)
	{
		char c = lex->input[lex->pos];
		int digit = -1;
		if (c >= '0' && c <= '9')
			digit = c - '0';
		else if (base == 16 && c >= 'a' && c <= 'f')
			digit = 10 + c - 'a';
		else if (base == 16 && c >= 'A' && c <= 'F')
			digit = 10 + c - 'A';
		if (digit < 0 || digit >= base)
			break;
		val = val * base + digit;
		lex->pos++;
	}
	lex->current.type = ATOK_NUM;
	lex->current.num_val = val;
	lex->current.start = lex->input + start;
	lex->current.len = lex->pos - start;
}

static void	lex_variable(t_arith_lexer *lex)
{
	int	start;

	start = lex->pos;
	while (lex->pos < lex->len && is_var_char(lex->input[lex->pos]))
		lex->pos++;
	lex->current.type = ATOK_VAR;
	lex->current.var_name = (char *)(lex->input + start);
	lex->current.var_len = lex->pos - start;
	lex->current.start = lex->input + start;
	lex->current.len = lex->pos - start;
}

static void	lex_two_char_op(t_arith_lexer *lex, char c1, char c2,
							t_arith_tok single, t_arith_tok dbl)
{
	if (lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == c2)
	{
		lex->current.type = dbl;
		lex->current.start = lex->input + lex->pos;
		lex->current.len = 2;
		lex->pos += 2;
	}
	else
	{
		lex->current.type = single;
		lex->current.start = lex->input + lex->pos;
		lex->current.len = 1;
		lex->pos++;
	}
	(void)c1;
}

static void	lex_operator(t_arith_lexer *lex)
{
	char	c;

	c = lex->input[lex->pos];
	lex->current.start = lex->input + lex->pos;
	lex->current.len = 1;
	if (c == '+')
		{ lex->current.type = ATOK_PLUS; lex->pos++; }
	else if (c == '-')
		{ lex->current.type = ATOK_MINUS; lex->pos++; }
	else if (c == '*')
	{
		/* Check for ** (exponentiation) */
		if (lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == '*')
		{
			lex->current.type = ATOK_POW;
			lex->current.len = 2;
			lex->pos += 2;
		}
		else
		{
			lex->current.type = ATOK_MUL;
			lex->pos++;
		}
	}
	else if (c == '/')
		{ lex->current.type = ATOK_DIV; lex->pos++; }
	else if (c == '%')
		{ lex->current.type = ATOK_MOD; lex->pos++; }
	else if (c == '(')
		{ lex->current.type = ATOK_LPAREN; lex->pos++; }
	else if (c == ')')
		{ lex->current.type = ATOK_RPAREN; lex->pos++; }
	else if (c == '?')
		{ lex->current.type = ATOK_TERNQ; lex->pos++; }
	else if (c == ':')
		{ lex->current.type = ATOK_TERNC; lex->pos++; }
	else if (c == ',')
		{ lex->current.type = ATOK_COMMA; lex->pos++; }
	else if (c == '~')
		{ lex->current.type = ATOK_BNOT; lex->pos++; }
	else if (c == '^')
		{ lex->current.type = ATOK_BXOR; lex->pos++; }
	else if (c == '=')
		lex_two_char_op(lex, '=', '=', ATOK_ASSIGN, ATOK_EQ);
	else if (c == '!')
		lex_two_char_op(lex, '!', '=', ATOK_NOT, ATOK_NE);
	else if (c == '<')
	{
		if (lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == '=')
			{ lex->current.type = ATOK_LE; lex->current.len = 2; lex->pos += 2; }
		else if (lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == '<')
			{ lex->current.type = ATOK_LSHIFT; lex->current.len = 2; lex->pos += 2; }
		else
			{ lex->current.type = ATOK_LT; lex->pos++; }
	}
	else if (c == '>')
	{
		if (lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == '=')
			{ lex->current.type = ATOK_GE; lex->current.len = 2; lex->pos += 2; }
		else if (lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == '>')
			{ lex->current.type = ATOK_RSHIFT; lex->current.len = 2; lex->pos += 2; }
		else
			{ lex->current.type = ATOK_GT; lex->pos++; }
	}
	else if (c == '&')
		lex_two_char_op(lex, '&', '&', ATOK_BAND, ATOK_AND);
	else if (c == '|')
		lex_two_char_op(lex, '|', '|', ATOK_BOR, ATOK_OR);
	else
	{
		lex->current.type = ATOK_ERROR;
		lex->error = true;
		lex->pos++;
	}
}

void	arith_lexer_init(t_arith_lexer *lex, const char *input, int len)
{
	lex->input = input;
	lex->pos = 0;
	lex->len = len;
	lex->error = false;
	lex->error_msg = NULL;
	lex->current.type = ATOK_EOF;
	arith_lexer_advance(lex);
}

void	arith_lexer_advance(t_arith_lexer *lex)
{
	char	c;

	skip_whitespace(lex);
	if (lex->pos >= lex->len)
	{
		lex->current.type = ATOK_EOF;
		lex->current.start = lex->input + lex->pos;
		lex->current.len = 0;
		return;
	}
	c = lex->input[lex->pos];
	if (ft_isdigit(c))
		lex_number(lex);
	else if (is_var_start(c))
		lex_variable(lex);
	else
		lex_operator(lex);
}

t_arith_token	arith_lexer_peek(t_arith_lexer *lex)
{
	return (lex->current);
}
