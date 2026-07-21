/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   redirect_herestring.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include <fcntl.h>
#include <unistd.h>

/* <<< word: the word expands with ASSIGNMENT semantics (parameter and
   command substitution, no field splitting, no globbing — `x="a b";
   cat <<< $x` prints "a b" like bash), a newline is appended, and the
   result becomes stdin. Small payloads ride an anonymous pipe; anything
   that could exceed the pipe buffer goes through an unlinked temp file
   so the shell can never deadlock writing its own redirect. */

/* Back `text` (len bytes + a newline) with a readable fd. */
static int	herestring_fd(const char *text, size_t len)
{
	char	path[64];
	int		p[2];
	int		fd;

	if (len < 60000)
	{
		if (pipe(p) != 0)
			return (-1);
		if (write(p[1], text, len) < 0 || write(p[1], "\n", 1) < 0)
			return (close(p[0]), close(p[1]), -1);
		close(p[1]);
		return (p[0]);
	}
	ft_strlcpy(path, "/tmp/.hellish_hs_XXXXXX", sizeof(path));
	fd = mkstemp(path);
	if (fd < 0)
		return (-1);
	unlink(path);
	if (write(fd, text, len) < 0 || write(fd, "\n", 1) < 0)
		return (close(fd), -1);
	lseek(fd, 0, SEEK_SET);
	return (fd);
}

/* Expand the target word and register the redirect, mirroring what
   try_create_redir does for file redirects: the t_redir carries the
   backing fd directly (no filename to open later). */
int	herestring_redir(t_shell *state, t_ast_node *curr, int src_fd)
{
	t_vec	args;
	t_redir	rd;
	char	*val;

	vec_init(&args);
	args.elem_size = sizeof(char *);
	if (curr->children.len < 2)
		return (-1);
	expand_word_assign_ro(state,
		&((t_ast_node *)curr->children.ctx)[1], &args);
	val = NULL;
	if (args.len)
		val = ((char **)args.ctx)[0];
	if (!val)
		val = ft_strdup("");
	rd = create_redir(true, src_fd, false);
	rd.fd = herestring_fd(val, ft_strlen(val));
	xfree(val);
	xfree(args.ctx);
	if (rd.fd < 0)
		return (-1);
	curr->redir_idx = state->redirects.len;
	curr->has_redirect = true;
	vec_push(&state->redirects, &rd);
	return (0);
}
