/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   create_redir.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:27:47 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:27:47 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

bool	create_redir(t_tt tt, char *fname, t_redir *ret, int src_fd)
{
	ft_assert(tt != TT_HEREDOC && "HEREDOCS are handled separately");
	ret->fname = fname;
	ret->direction_in = tt == TT_REDIRECT_LEFT;
	ret->src_fd = src_fd;
	if (!ret->fname)
		return (false);
	if (tt == TT_REDIRECT_LEFT)
		ret->fd = open(ret->fname, O_RDONLY);
	else if (tt == TT_REDIRECT_RIGHT)
		ret->fd = open(ret->fname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	else if (tt == TT_APPEND)
		ret->fd = open(ret->fname, O_WRONLY | O_CREAT | O_APPEND, 0666);
	if (ret->fd < 0)
		return (false);
	ret->should_delete = false;
	return (true);
}
