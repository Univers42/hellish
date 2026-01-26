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

static bool	create_dup_redir(t_tt tt, char *fname, t_redir *ret, int src_fd)
{
	int	target_fd;

	ret->fname = fname;
	ret->src_fd = src_fd;
	ret->should_delete = false;
	ret->direction_in = (tt == TT_DUP_IN);
	if (!fname || !*fname)
		return (false);
	if (fname[0] == '-' && fname[1] == '\0')
		return (ret->fd = -1, ret->close_fd = true, true);
	target_fd = ft_atoi(fname);
	if (target_fd < 0)
		return (false);
	ret->fd = dup(target_fd);
	ret->close_fd = false;
	return (ret->fd >= 0);
}

static bool	open_file_redir(t_tt tt, t_redir *ret)
{
	if (tt == TT_REDIRECT_LEFT)
		ret->fd = open(ret->fname, O_RDONLY);
	else if (tt == TT_REDIRECT_RIGHT)
		ret->fd = open(ret->fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	else if (tt == TT_APPEND)
		ret->fd = open(ret->fname, O_WRONLY | O_CREAT | O_APPEND, 0644);
	else
		ret->fd = -1;
	if (ret->fd < 0)
		return (false);
	ret->should_delete = false;
	return (true);
}

static bool	handle_devfd_redir(char *fname, t_redir *ret)
{
	int	orig_fd;

	orig_fd = ft_atoi(fname + 8);
	if (orig_fd < 0)
		return (false);
	ret->fd = dup(orig_fd);
	ret->should_delete = false;
	return (ret->fd >= 0);
}

bool	create_redir_4(t_tt tt, char *fname, t_redir *ret, int src_fd)
{
	ft_assert(tt != TT_HEREDOC && "HEREDOCS are handled separately");
	ret->fname = fname;
	ret->direction_in = (tt == TT_REDIRECT_LEFT || tt == TT_DUP_IN);
	ret->src_fd = src_fd;
	ret->close_fd = false;
	if (!ret->fname)
		return (false);
	if (tt == TT_DUP_OUT || tt == TT_DUP_IN)
		return (create_dup_redir(tt, fname, ret, src_fd));
	if (ft_strncmp(fname, "/dev/fd/", 8) == 0)
		return (handle_devfd_redir(fname, ret));
	return (open_file_redir(tt, ret));
}

bool	create_redir_heredoc(int heredoc_fd, t_redir *ret)
{
	if (heredoc_fd < 0)
		return (false);
	ret->fd = heredoc_fd;
	ret->fname = NULL;
	ret->src_fd = STDIN_FILENO;
	ret->should_delete = false;
	ret->direction_in = true;
	ret->close_fd = false;
	return (ret->fd >= 0);
}
