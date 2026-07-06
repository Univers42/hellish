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

/* `n>&m` / `n<&m`: the source fd m must actually be open — bash fails the
   whole redirection ("m: Bad file descriptor", status 1, command not run)
   rather than letting dup2 fail silently at apply time, so we probe it
   with fcntl here where a false return already means "redirect failed". */
static bool	create_dup_redir(t_tt tt, char *fname, t_redir *ret, int src_fd)
{
	int	target_fd;

	ret->fname = fname;
	ret->src_fd = src_fd;
	ret->should_delete = false;
	ret->direction_in = (tt == TT_DUP_IN);
	ret->is_dup = true;
	if (!fname || !*fname)
		return (false);
	if (fname[0] == '-' && fname[1] == '\0')
		return (ret->fd = -1, ret->close_fd = true, true);
	target_fd = ft_atoi(fname);
	if (target_fd < 0 || fcntl(target_fd, F_GETFD) < 0)
		return (false);
	ret->fd = target_fd;
	ret->close_fd = false;
	return (true);
}

/* open(2) hands back the lowest free fd, which can be the very fd this
   redirect targets (e.g. `exec 3> f` while 3 is the lowest hole).  The
   apply step would then be a no-op and the per-command teardown's
   close(redir.fd) would tear down the fd it just installed.  Move the
   scratch fd up to >= 10 (clear of the user-addressable low range, the
   same trick bash uses) so it is always distinct from src_fd. */
static bool	open_file_redir(t_tt tt, t_redir *ret)
{
	if (tt == TT_REDIRECT_LEFT)
		ret->fd = open(ret->fname, O_RDONLY);
	else if (tt == TT_REDIRECT_RIGHT || tt == TT_CLOBBER)
		ret->fd = open(ret->fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	else if (tt == TT_APPEND)
		ret->fd = open(ret->fname, O_WRONLY | O_CREAT | O_APPEND, 0644);
	else if (tt == TT_READWRITE)
		ret->fd = open(ret->fname, O_RDWR | O_CREAT, 0644);
	else
		ret->fd = -1;
	if (ret->fd < 0)
		return (false);
	if (ret->fd == ret->src_fd)
	{
		ret->fd = fcntl(ret->src_fd, F_DUPFD, 10);
		close(ret->src_fd);
		if (ret->fd < 0)
			return (false);
	}
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
	if (ret->fd == ret->src_fd)
	{
		ret->fd = fcntl(ret->src_fd, F_DUPFD, 10);
		close(ret->src_fd);
	}
	ret->should_delete = false;
	return (ret->fd >= 0);
}

/* Build a t_redir from a redirect token type, filename, and source fd.
   Dispatch: &fd (dup) → create_dup_redir; /dev/fd/N (process subst result)
   → handle_devfd_redir (dup the existing fd rather than re-opening it);
   everything else → open_file_redir.  Heredocs are asserted out here since
   they follow a completely different path (materialize_heredoc). */
bool	create_redir_4(t_tt tt, char *fname, t_redir *ret, int src_fd)
{
	ft_assert(tt != TT_HEREDOC && "HEREDOCS are handled separately");
	ret->fname = fname;
	ret->direction_in = (tt == TT_REDIRECT_LEFT || tt == TT_DUP_IN
			|| tt == TT_READWRITE);
	ret->src_fd = src_fd;
	ret->close_fd = false;
	ret->is_dup = false;
	if (!ret->fname)
		return (false);
	if (tt == TT_DUP_OUT || tt == TT_DUP_IN)
		return (create_dup_redir(tt, fname, ret, src_fd));
	if (ft_strncmp(fname, "/dev/fd/", 8) == 0)
		return (handle_devfd_redir(fname, ret));
	return (open_file_redir(tt, ret));
}

/* Wrap a pre-created heredoc fd into a t_redir.  The fd is already open for
   reading (or pointing at a pipe) when this is called; we just fill the
   struct fields correctly so the executor can dup2 it to stdin. */
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
	ret->is_dup = false;
	return (ret->fd >= 0);
}
