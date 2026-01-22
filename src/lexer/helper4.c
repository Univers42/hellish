/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helper4.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 10:13:59 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 10:15:35 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

int	create_token_consume(char *start, int fd_len, t_tt tt, t_token *out)
{
	*out = create_token(start, fd_len, tt);
	return (fd_len);
}

/* Check if string starts with a fd-prefixed redirect like 2> or 1>> */
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
	if (*p == '>' && *(p + 1) == '>')
		return (create_token_consume(str, fd_len + 2, TT_APPEND, out));
	if (*p == '>')
		return (create_token_consume(str, fd_len + 1, TT_REDIRECT_RIGHT, out));
	if (*p == '<' && *(p + 1) == '<')
		return (create_token_consume(str, fd_len + 2, TT_HEREDOC, out));
	if (*p == '<')
		return (create_token_consume(str, fd_len + 1, TT_REDIRECT_LEFT, out));
	return (0);
}
