/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_eval.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "executor.h"
#include <fcntl.h>

/* Join argv[from..] with single spaces — the result is what gets fed to the
   parser. We build it piece by piece with ft_strjoin rather than computing
   the total length upfront because the argv strings can come from wildcard
   expansion and we do not know their lengths cheaply. Slightly slower but
   avoids a two-pass loop. */
static char	*join_args(t_vec argv, size_t from)
{
	char	**av;
	char	*acc;
	char	*tmp;
	size_t	i;

	av = (char **)argv.ctx;
	acc = ft_strdup("");
	i = from;
	while (i < argv.len)
	{
		tmp = ft_strjoin(acc, av[i]);
		xfree(acc);
		acc = tmp;
		if (++i < argv.len)
		{
			tmp = ft_strjoin(acc, " ");
			xfree(acc);
			acc = tmp;
		}
	}
	return (acc);
}

/* eval: concatenate the arguments (space-separated), then parse and execute
   the resulting string in the current shell context. Must be a builtin for
   the same reason as source: assignments, function definitions, and option
   changes made inside eval must be visible to the caller. */
int	builtin_eval(t_shell *state, t_vec argv)
{
	char	*joined;
	int		status;

	if (argv.len < 2)
		return (0);
	joined = join_args(argv, 1);
	status = exec_string(state, joined);
	xfree(joined);
	return (status);
}

/* . file (a.k.a. source): read the whole file into memory, then execute it
   in this shell's context. Reading upfront (rather than line-by-line) means
   a sourced file that redefines itself mid-execution still runs its original
   text — same behaviour as bash. No PATH search is done; the argument is
   used as-is (use `source` for the bash-style PATH-aware form if needed). */
int	builtin_source(t_shell *state, t_vec argv)
{
	char		**av;
	t_string	buf;
	char		*content;
	int			fd;
	int			status;

	av = (char **)argv.ctx;
	if (argv.len < 2)
		return (ft_eprintf("%s: .: filename argument required\n",
				state->ctx), 2);
	fd = open(av[1], O_RDONLY);
	if (fd < 0)
		return (ft_eprintf("%s: .: %s: No such file or directory\n",
				state->ctx, av[1]), 1);
	vec_init(&buf);
	buf.elem_size = 1;
	vec_append_fd(fd, &buf);
	close(fd);
	content = ft_strndup((char *)buf.ctx, buf.len);
	xfree(buf.ctx);
	status = exec_string(state, content);
	xfree(content);
	return (status);
}
