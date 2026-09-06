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
#include "ft_builtins.h"
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
   changes made inside eval must be visible to the caller. A leading `--`
   is an options terminator, not text: bash-completion 2.14 writes
   `eval -- "$1=()"` throughout, and joining the -- in made it a command
   ("--: command not found" at every login, #105). */
int	builtin_eval(t_shell *state, t_vec argv)
{
	char	*joined;
	int		status;
	size_t	from;

	from = 1;
	if (argv.len > 1 && ft_strcmp(((char **)argv.ctx)[1], "--") == 0)
		from = 2;
	if (argv.len <= from)
		return (0);
	joined = join_args(argv, from);
	status = exec_string(state, joined);
	xfree(joined);
	return (status);
}

/* . file (a.k.a. source): read the whole file into memory, then execute it
   in this shell's context. Reading upfront (rather than line-by-line) means
   a sourced file that redefines itself mid-execution still runs its original
   text — same behaviour as bash. No PATH search is done; the argument is
   used as-is (use `source` for the bash-style PATH-aware form if needed). */
/* Run the sourced text, temporarily binding extra arguments as $1..$N
   the way bash/ksh/zsh do: `. file a b` gives the file its own
   positionals and RESTORES the caller's afterwards; with no extra args
   the file shares the caller's parameters (and `set` inside persists).
   exec_file_string rather than exec_string so a parse error in the file
   names the file and the line (error_where.c).

   A NULL content is an EMPTY file: nothing was read, so the vector in
   builtin_source never allocated and ft_strndup(NULL, 0) handed back
   NULL, which the parser then walked off.  `. /dev/null` is not a
   curiosity -- it is how a script sources an optional config that may
   not exist yet -- and it took the shell down with SIGSEGV, in every
   release.  An empty file sources to nothing, status 0, as in bash. */
static int	run_source_pos(t_shell *state, char *content, t_vec argv)
{
	t_pos	saved;
	int		status;

	if (!content)
		return (0);
	if (argv.len <= 2)
		return (exec_file_string(state, content, ((char **)argv.ctx)[1]));
	saved = state->pos;
	state->pos = (t_pos){0};
	pos_build(&state->pos, (char **)argv.ctx + 2, argv.len - 2);
	status = exec_file_string(state, content, ((char **)argv.ctx)[1]);
	pos_free(&state->pos);
	state->pos = saved;
	pos_set_cnt(&state->pos);
	return (status);
}

static int	run_source(t_shell *state, char *content, t_vec argv)
{
	char	*path;
	char	*zero;
	int		status;

	path = ((char **)argv.ctx)[1];
	state->source_depth++;
	frame_push(state, NULL, path);
	if (zsh_path(path))
		zsh_mode_swap(state, true);
	zero = zsh_zero_bind(state, path);
	status = run_source_pos(state, content, argv);
	zsh_zero_restore(state, zero);
	frame_pop(state);
	state->source_depth--;
	return (status);
}

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
	if (omz_loader_path(av[1]))
		return (close(fd), omz_shim(state, av[1]));
	vec_init(&buf);
	buf.elem_size = 1;
	vec_append_fd(fd, &buf);
	close(fd);
	content = ft_strndup((char *)buf.ctx, buf.len);
	xfree(buf.ctx);
	status = run_source(state, content, argv);
	xfree(content);
	return (status);
}
