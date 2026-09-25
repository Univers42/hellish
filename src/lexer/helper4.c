/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helper4.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 10:13:59 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include <limits.h>

/* Build a token of type tt spanning fd_len bytes starting at start, store it
   in *out, and return fd_len so callers can use it as both token length and
   "how many bytes to skip" in one expression. */
int	create_token_consume(char *start, int fd_len, t_tt tt, t_token *out)
{
	*out = create_token(start, fd_len, tt);
	return (fd_len);
}

/* Given that str[0..fd_len-1] are digits and p points at the redirect char,
   pick the right token type (including two-char forms like >& and <<) and
   return the total span length. Longer forms are checked first so `>>`
   wins over `>` when fd_len digits precede the `>>`, and `<<-` over `<<`:
   without it `3<<-EOF` lexed as `3<<` and the delimiter `-EOF`, so the body
   ran to end of file. */
static int	fd_redir_type(char *str, char *p, int fd_len, t_token *out)
{
	if (*p == '>' && *(p + 1) == '&')
		return (create_token_consume(str, fd_len + 2, TT_DUP_OUT, out));
	if (*p == '<' && *(p + 1) == '&')
		return (create_token_consume(str, fd_len + 2, TT_DUP_IN, out));
	if (*p == '>' && *(p + 1) == '>')
		return (create_token_consume(str, fd_len + 2, TT_APPEND, out));
	if (*p == '>' && *(p + 1) == '|')
		return (create_token_consume(str, fd_len + 2, TT_CLOBBER, out));
	if (*p == '>')
		return (create_token_consume(str, fd_len + 1, TT_REDIRECT_RIGHT, out));
	if (*p == '<' && *(p + 1) == '<' && *(p + 2) == '<')
		return (create_token_consume(str, fd_len + 3, TT_HERESTRING, out));
	if (*p == '<' && *(p + 1) == '<' && *(p + 2) == '-')
		return (create_token_consume(str, fd_len + 3, TT_HEREDOC, out));
	if (*p == '<' && *(p + 1) == '<')
		return (create_token_consume(str, fd_len + 2, TT_HEREDOC, out));
	if (*p == '<' && *(p + 1) == '>')
		return (create_token_consume(str, fd_len + 2, TT_READWRITE, out));
	if (*p == '<')
		return (create_token_consume(str, fd_len + 1, TT_REDIRECT_LEFT, out));
	return (0);
}

/* The length of the IO_NUMBER at s, or 0. POSIX 2.10.1: a token made of
   digits alone, delimited by `<` or `>`, is the fd the redirection acts
   on. bash also wants the number to fit an int, and otherwise leaves it a
   word: `echo x 99999999999>f` writes "x 99999999999" to f. There is no
   cap on the digits below that -- `255>f` is fd 255, and `1234567>f` a
   "Bad file descriptor" at run time, not the word `12` and a `34567>`. */
static int	io_number_len(const char *s)
{
	long	v;
	int		i;

	v = 0;
	i = 0;
	while (ft_isdigit((unsigned char)s[i]))
	{
		v = v * 10 + (s[i++] - '0');
		if (v > INT_MAX)
			return (0);
	}
	if (i > 0 && (s[i] == '<' || s[i] == '>'))
		return (i);
	return (0);
}

/* A token that starts at s is an IO_NUMBER. Only a token START can be
   one: `a2>f`, `x=2>f` and `"x"12>f` are the words `a2`, `x=2` and `x12`
   followed by `>f`, as in bash -- they used to be cut before the digits,
   which sent stderr to f and dropped the digits from the word. Right after
   `>&` or `<&` the digits are the descriptor dup'd, never a new
   redirection's: `>&2>f` is `>&2` then `>f` (bash's last_read_token rule),
   not a syntax error. */
int	io_number_at(t_deque_tok *toks, const char *s)
{
	t_ltoken	*last;
	int			n;

	n = io_number_len(s);
	if (n == 0 || toks->deqtok.len == 0)
		return (n);
	last = (t_ltoken *)deque_idx(&toks->deqtok, toks->deqtok.len - 1);
	if (last->tt == TT_DUP_IN || last->tt == TT_DUP_OUT)
		return (0);
	return (n);
}

/* Emit the fd-prefixed redirect (e.g. 2>, 10>>, 255<&) at the start of
   str. parse_op calls this where a token starts, and tokenize_step routes
   there only an IO_NUMBER (io_number_at). Returns 0 if there is none,
   otherwise the total span length, with *out filled. */
int	check_fd_redirect(char *str, t_token *out)
{
	int	fd_len;

	fd_len = io_number_len(str);
	if (fd_len == 0)
		return (0);
	return (fd_redir_type(str, str + fd_len, fd_len, out));
}
