/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helper4.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 10:13:59 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 14:07:05 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

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
   wins over `>` when fd_len digits precede the `>>`. */
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
	if (*p == '<' && *(p + 1) == '<')
		return (create_token_consume(str, fd_len + 2, TT_HEREDOC, out));
	if (*p == '<' && *(p + 1) == '>')
		return (create_token_consume(str, fd_len + 2, TT_READWRITE, out));
	if (*p == '<')
		return (create_token_consume(str, fd_len + 1, TT_REDIRECT_LEFT, out));
	return (0);
}

/* Check whether str starts with an fd-prefixed redirect (e.g. 2>, 1>>).
   We allow 1-2 digit fd numbers -- 0, 1, 2 are the common ones; bash also
   handles 9. We reject fd_len > 2 to avoid eating a decimal integer that
   happens to precede a `>` in an expression context. Returns 0 if no match,
   otherwise the total span length and fills *out with the token. */
int	check_fd_redirect(char *str, t_token *out)
{
	int		fd_len;
	char	*p;

	fd_len = 0;
	p = str;
	while (*p && ft_isdigit((unsigned char)*p))
	{
		fd_len++;
		p++;
	}
	if (fd_len == 0 || fd_len > 2)
		return (0);
	return (fd_redir_type(str, p, fd_len, out));
}
