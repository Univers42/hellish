/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 17:12:27 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* Detect a numeric prefix (0x/0X → hex, leading 0+digit → octal, else
   decimal). Sets *new_pos to the first digit after the prefix and returns
   the base. Called only AFTER an initial decimal scan found no '#', so the
   two-pass design keeps the common decimal case cheap. */
static int	get_base(const char *input, int pos, int len, int *new_pos)
{
	if (input[pos] == '0' && pos + 1 < len)
	{
		if (input[pos + 1] == 'x' || input[pos + 1] == 'X')
		{
			*new_pos = pos + 2;
			return (16);
		}
		else if (ft_isdigit(input[pos + 1]))
		{
			*new_pos = pos + 1;
			return (8);
		}
	}
	*new_pos = pos;
	return (10);
}

/* Convert one character to its numeric value in the given base, returning -1
   if the character is out of range. Only hex (base 16) understands a-f/A-F;
   bases 2..10 accept digits only. Tight on purpose — no locale overhead. */
static int	get_digit(char c, int base)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	if (base == 16 && c >= 'a' && c <= 'f')
		return (10 + c - 'a');
	if (base == 16 && c >= 'A' && c <= 'F')
		return (10 + c - 'A');
	return (-1);
}

/* Consume a run of digits in `base` starting at *pos and return their value.
   Advances *pos past the last accepted digit. Used for both the C-style
   0x/0-prefix path and the bash base#digits path. */
long long	parse_digits(const char *input, int *pos, int len, int base)
{
	long long	val;
	int			digit;

	val = 0;
	while (*pos < len)
	{
		digit = get_digit(input[*pos], base);
		if (digit < 0 || digit >= base)
			break ;
		val = val * base + digit;
		(*pos)++;
	}
	return (val);
}

/* Lex an integer literal into lex->current. The tricky bit is that we must
   peek ahead to decide the base: "0x…" is hex, a bare "0" followed by more
   digits is octal, "N#…" is bash's arbitrary-base notation, and everything
   else is plain decimal. We parse the leading decimal digits first (needed
   to read the base for N#), then branch. The start/len in the token cover
   the whole literal including prefix and base spec. */
void	lex_number(t_arith_lexer *lex)
{
	int			start;
	int			base;
	long long	val;
	int			pos;

	start = lex->pos;
	pos = lex->pos;
	val = parse_digits(lex->input, &pos, lex->len, 10);
	if (pos < lex->len && lex->input[pos] == '#')
		val = parse_base_n(lex, &pos, val);
	else
	{
		base = get_base(lex->input, lex->pos, lex->len, &pos);
		val = parse_digits(lex->input, &pos, lex->len, base);
	}
	lex->current.type = ATOK_NUM;
	lex->current.num_val = val;
	lex->current.start = lex->input + start;
	lex->current.len = pos - start;
	lex->pos = pos;
}

/* Advance the lexer by one token, storing the new current token in
   lex->current. When a cached token array is present the advance delegates
   to arith_advance_toks (replay mode); otherwise it scans the input string.
   The dispatch is transparent to every caller, so the parser doesn't know
   whether it's reading fresh characters or replaying a pre-lexed array. */
void	arith_lexer_advance(t_arith_lexer *lex)
{
	char	c;

	if (lex->toks)
	{
		arith_advance_toks(lex);
		return ;
	}
	skip_whitespace(lex);
	if (lex->pos >= lex->len)
	{
		lex->current.type = ATOK_EOF;
		lex->current.start = lex->input + lex->pos;
		lex->current.len = 0;
		return ;
	}
	c = lex->input[lex->pos];
	if (ft_isdigit(c))
		lex_number(lex);
	else if (is_var_start(c))
		lex_variable(lex);
	else if (c == '$')
		lex_dollar_var(lex);
	else
		lex_operator(lex);
}
