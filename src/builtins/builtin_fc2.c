/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_fc2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "history.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int	fc_run_editor(t_shell *state, const char *editor, char *tmpf)
{
	pid_t	pid;
	int		st;
	int		fd;

	pid = fork();
	if (pid == 0)
		(execl(editor, editor, tmpf, NULL), exit(127));
	waitpid(pid, &st, 0);
	if (WIFEXITED(st) && WEXITSTATUS(st) == 0)
	{
		fd = open(tmpf, O_RDONLY);
		if (fd >= 0)
		{
			vec_init(&state->input);
			state->input.elem_size = 1;
			vec_append_fd(fd, &state->input);
			close(fd);
		}
	}
	unlink(tmpf);
	return (0);
}

int	fc_edit_run(t_shell *state, const char *editor, int first, int last)
{
	char	tmpf[64];

	if (fc_write_tmp(state, tmpf, first, last))
		return (1);
	return (fc_run_editor(state, editor, tmpf));
}

static const char	*get_fc_editor(t_shell *state)
{
	char	*ed;

	ed = env_expand(state, "FCEDIT");
	if (ed && *ed)
		return (ed);
	ed = env_expand(state, "EDITOR");
	if (ed && *ed)
		return (ed);
	return ("/bin/ed");
}

static int	fc_run(t_shell *state, char **av, int ac)
{
	int			first;
	int			last;
	const char	*editor;

	if (ac >= 3 && ft_strcmp(av[1], "-e") == 0)
		editor = av[2];
	else
		editor = get_fc_editor(state);
	if (ac > 1 && av[1][0] != '-')
		first = fc_resolve_idx(state, av[1]);
	else
		first = fc_resolve_idx(state, NULL);
	if (ac > 2 && av[2][0] != '-')
		last = fc_resolve_idx(state, av[2]);
	else
		last = fc_resolve_idx(state, NULL);
	return (fc_edit_run(state, editor, first, last));
}

int	builtin_fc(t_shell *state, t_vec argv)
{
	char	**av;
	int		ac;

	av = (char **)argv.ctx;
	ac = (int)argv.len;
	if (!state->hist.hist_active || !state->hist.hist_cmds.len)
		return (ft_eprintf("%s: fc: no command history\n", state->ctx), 1);
	if (ac >= 2 && ft_strcmp(av[1], "-l") == 0)
		return (fc_list(state, av + 2, ac - 2, false));
	if (ac >= 2 && ft_strcmp(av[1], "-lr") == 0)
		return (fc_list(state, av + 2, ac - 2, true));
	return (fc_run(state, av, ac));
}
