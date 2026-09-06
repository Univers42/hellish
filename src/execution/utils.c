/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:07:53 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:25:02 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "sys.h"

/* Save a standard fd (0/1/2) for later restore around a redirected builtin
   or in-process compound.  We MUST land the copy at a HIGH fd (>=10), never
   plain dup(): dup() returns the lowest free descriptor -- 5, 6, 7 -- which
   is exactly the range scripts claim with `exec 5>file`.  autoconf's
   configure holds config.log on fd 5 and the original stdout on fd 6, so a
   plain-dup backup silently clobbers them and every `>&5`/`>&6` then fails
   with EBADF (the shell can't build config.log and the compiler probe dies).
   fcntl(F_DUPFD_CLOEXEC, 10) keeps our book-keeping fds out of the 3..9
   window and out of child processes.  Falls back to dup() if unsupported. */
int	save_fd(int fd)
{
	int	saved;

	saved = fcntl(fd, F_DUPFD_CLOEXEC, 10);
	if (saved < 0)
		saved = dup(fd);
	return (saved);
}

/* Apply one resolved t_redir to the process's fd table.  Three cases:
   close_fd -- just close the fd (2>&- syntax);
   is_dup   -- dup2 src_fd to a different target without closing the source
               (used when the source is a pre-opened read fd like a pipe);
   normal   -- dup2 then close the original so only the canonical fd number
               remains live (the typical file-backed redirect).
   `both` is the `&>file` / `>&file` shorthand: bash defines it as
   `>file 2>&1`, so stderr follows stdout AFTER stdout has moved. Doing it
   here rather than emitting a second t_redir keeps one AST redirect node
   mapping to one table entry, which every caller already assumes. */
static void	apply_redir_now(t_redir redir)
{
	if (redir.close_fd)
	{
		close(redir.src_fd);
		return ;
	}
	if (redir.is_dup)
	{
		dup2(redir.fd, redir.src_fd);
		return ;
	}
	if (redir.fd != redir.src_fd)
	{
		dup2(redir.fd, redir.src_fd);
		close(redir.fd);
	}
	if (redir.both)
		dup2(redir.src_fd, STDERR_FILENO);
}

/* Look up the t_redir at index `idx` in state->redirects and apply it.
   Out-of-range access is an internal bug (the parser or heredoc collector
   produced a bad index) so we print an error and exit rather than silently
   reading garbage memory.
     Once applied, an owned entry is marked consumed (fd = -1) right here.
   The normal path closed the scratch fd it dup2'd from, and when the
   parked fd already IS the target (`exec 10>a`: parking starts at 10) the
   apply was a no-op and that fd now belongs to the script.  Either way
   free_executable_node and the end-of-cycle free_redirects must not close
   that NUMBER again -- for `exec 10>a` that tore the redirection down. */
void	apply_redir(t_shell *state, int idx)
{
	t_redir	*r;

	if (idx < 0 || !state->redirects.ctx
		|| (size_t)idx >= state->redirects.len)
	{
		if (state->ctx)
			ft_eprintf(MSG_INT_ERR_REDIR_IDX, state->ctx, idx);
		else
			ft_eprintf(MSG_INT_ERR_REDIR_IDX, NAME, idx);
		exit(EXIT_GENERAL_ERR);
	}
	r = &((t_redir *)state->redirects.ctx)[(size_t)idx];
	apply_redir_now(*r);
	if (!r->close_fd && !r->is_dup)
		r->fd = -1;
}

/* Apply all redirects whose indices are stored in exe->redirs (the
   pre-collected path, used when we already resolved them in the parent
   before forking -- avoids resolving the same file twice). */
void	apply_redirs_from_vec(t_shell *state, t_executable_node *exe)
{
	size_t	i;
	int		idx;

	i = 0;
	while (i < exe->redirs.len)
	{
		idx = *(int *)vec_idx(&exe->redirs, i++);
		apply_redir(state, idx);
	}
}

/* Apply redirects directly from the AST (the lazy path, used in child
   processes where the parent did not pre-resolve).  We walk the node's
   children looking for AST_REDIRECT siblings.  An ambiguous redirect
   (variable expanded to multiple words) causes an error exit. */
void	apply_redirs_from_ast(t_shell *state, t_executable_node *exe)
{
	size_t		i;
	t_ast_node	*curr;
	int			idx;

	i = 0;
	while (++i < exe->node->children.len)
	{
		curr = (t_ast_node *)vec_idx(&exe->node->children, i);
		if (curr->node_type != AST_REDIRECT)
			continue ;
		if (redirect_from_ast_redir(state, curr, &idx))
		{
			ft_eprintf(MSG_AMBIGUOUS_REDIR, state->ctx);
			exit(EXIT_GENERAL_ERR);
		}
		apply_redir(state, idx);
	}
}
