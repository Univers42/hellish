/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cmdsub_fast.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sh_input.h"
#include "sh_alias.h"
#include <fcntl.h>

int		exec_string(t_shell *state, char *cmd);
bool	csf_eligible(t_shell *state, const char *s);

/* Forkless $(...) fast path.  A fork exists to ISOLATE the substitution's
   side effects; when the body provably has none — a single whitelisted
   builtin (echo/printf/pwd/true/:), no redirections or control operators,
   no ${v=...} assignment expansion, no $((...)) (whose assignments would
   leak), no -u/-e modes that could abort the shell mid-expansion — the
   body can run right here, with stdout parked on an unlinked temp file.
   That removes fork+COW+waitpid from the hottest substitution shapes
   (`$(echo $x)` and friends): the same trick that makes ksh93's command
   substitution famously fast, applied to the safe subset only.  Returns
   NULL when not eligible so the caller falls back to the fork. */

/* Quoted spans for the eligibility scan: '...' is fully opaque; inside
   "..." the metacharacters are literal (safe) but backticks and $((...))
   stay live (reject) and nested $( ) / ${ } need their own checks. */
int	csf_skip_quoted(const char *s, int i)
{
	char	q;

	q = s[i];
	i++;
	while (s[i] && s[i] != q)
	{
		if (q == '"' && (s[i] == '`'
				|| (s[i] == '$' && s[i + 1] == '(' && s[i + 2] == '(')))
			return (-1);
		if (q == '"' && s[i] == '$' && s[i + 1] == '(')
			i = csf_skip_csub(s, i);
		else if (q == '"' && s[i] == '$' && s[i + 1] == '{')
			i = csf_skip_param(s, i);
		else
			i++;
		if (i < 0)
			return (-1);
	}
	if (!s[i])
		return (-1);
	return (i + 1);
}

/* Redirect stdout into an unlinked temp file (no deadlock at any output
   size, unlike a pipe written and read by the same process). */
static int	csf_stdout_to_tmp(int *save1)
{
	char	tmpl[32];
	int		fd;

	ft_strlcpy(tmpl, "/tmp/.hellish_csXXXXXX", sizeof(tmpl));
	fd = mkstemp(tmpl);
	if (fd < 0)
		return (-1);
	unlink(tmpl);
	*save1 = fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC, 10);
	if (*save1 < 0)
		*save1 = dup(STDOUT_FILENO);
	if (*save1 < 0)
		return (close(fd), -1);
	dup2(fd, STDOUT_FILENO);
	return (fd);
}

/* Slurp the capture file back and strip the POSIX trailing newlines. */
static char	*csf_collect(int fd)
{
	t_string	out;
	char		*ret;
	size_t		nlen;

	lseek(fd, 0, SEEK_SET);
	vec_init(&out);
	out.elem_size = 1;
	vec_append_fd(fd, &out);
	close(fd);
	ret = xmalloc(out.len + 1);
	if (!ret)
		return (xfree(out.ctx), ft_strdup(""));
	if (out.len)
		ft_memcpy(ret, out.ctx, out.len);
	nlen = out.len;
	while (nlen > 0 && ret[nlen - 1] == '\n')
		nlen--;
	ret[nlen] = '\0';
	return (xfree(out.ctx), ret);
}

/* Run the eligible body in-process.  $? bookkeeping is saved around the
   call so only last_cmdsub_status (what $? means INSIDE the substitution
   result handling) reflects the body, exactly like the forked path. */
char	*cmdsub_fast(t_shell *state, const char *cmd)
{
	t_execution_state	save_exe;
	char				*save_st;
	int					fd;
	int					save1;
	int					rc;

	if (!csf_eligible(state, cmd))
		return (NULL);
	fd = csf_stdout_to_tmp(&save1);
	if (fd < 0)
		return (NULL);
	save_exe = state->last_cmd_st_exe;
	save_st = state->last_cmd_st;
	rc = exec_string(state, (char *)cmd);
	state->last_cmd_st_exe = save_exe;
	state->last_cmd_st = save_st;
	state->last_cmdsub_status = rc & 0xFF;
	dup2(save1, STDOUT_FILENO);
	close(save1);
	return (csf_collect(fd));
}

/* Is the fast path's command word shadowed by an alias? The old gate
   bailed whenever ANY alias existed, which turned every `$(printf ...)`
   in a theme prompt into a fork the moment an rc defined its first
   alias -- ~270 forks per re-source of hellishrc_plugins' theme file,
   the "source got slow again" of issue #108. Only an alias on THIS word
   can change this body's meaning: eligibility already guarantees a
   single simple command, so no other name in the body sits in command
   position. */
bool	csf_word_aliased(t_shell *state, const char *s, int len)
{
	char	buf[16];

	if (len <= 0 || len >= (int) sizeof(buf))
		return (true);
	ft_memcpy(buf, s, len);
	buf[len] = '\0';
	return (alias_get(&state->aliases, buf) != NULL);
}
