/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_fc.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "history.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

static int	resolve_idx(t_shell *state, const char *s)
{
	int	n;
	int	total;

	total = (int)state->hist.hist_cmds.len;
	if (!s)
		return (total - 1);
	n = ft_atoi(s);
	if (n < 0)
		n = total + n;
	else
		n = n - 1;
	if (n < 0)
		n = 0;
	if (n >= total)
		n = total - 1;
	return (n);
}

static int	fc_list(t_shell *state, char **av, int ac, bool reverse)
{
	int		first;
	int		last;
	int		i;
	bool	no_nums;

	no_nums = false;
	first = resolve_idx(state, (ac > 0) ? av[0] : NULL);
	last = resolve_idx(state, (ac > 1) ? av[1] : NULL);
	if (first == last && ac < 2)
	{
		first = (int)state->hist.hist_cmds.len - 16;
		if (first < 0)
			first = 0;
		last = (int)state->hist.hist_cmds.len - 1;
	}
	i = -1;
	while (++i < ac)
		if (av[i] && ft_strcmp(av[i], "-n") == 0)
			no_nums = true;
	if (reverse && first < last)
		i = first, first = last, last = i;
	i = first - 1;
	while (++i <= last)
	{
		if (!no_nums)
			ft_printf("%d\t", i + 1);
		ft_printf("%s\n", ((char **)state->hist.hist_cmds.ctx)[i]);
	}
	return (0);
}

static int	fc_edit_run(t_shell *state, const char *editor, int first, int last)
{
	char	tmpf[64];
	int		fd;
	int		i;
	pid_t	pid;
	int		st;
	char	*entry;

	ft_strlcpy(tmpf, "/tmp/.hellish_fc_XXXXXX", 64);
	fd = mkstemp(tmpf);
	if (fd < 0)
		return (ft_eprintf("%s: fc: cannot create temp file\n",
				state->ctx), 1);
	i = first;
	while (i <= last)
	{
		entry = ((char **)state->hist.hist_cmds.ctx)[i];
		write(fd, entry, ft_strlen(entry));
		write(fd, "\n", 1);
		i++;
	}
	close(fd);
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

int	builtin_fc(t_shell *state, t_vec argv)
{
	char		**av;
	int			ac;
	int			first;
	int			last;
	const char	*editor;

	av = (char **)argv.ctx;
	ac = (int)argv.len;
	if (!state->hist.hist_active || state->hist.hist_cmds.len == 0)
		return (ft_eprintf("%s: fc: no command history\n", state->ctx), 1);
	if (ac >= 2 && ft_strcmp(av[1], "-l") == 0)
		return (fc_list(state, av + 2, ac - 2, false));
	if (ac >= 2 && ft_strcmp(av[1], "-lr") == 0)
		return (fc_list(state, av + 2, ac - 2, true));
	if (ac >= 3 && ft_strcmp(av[1], "-e") == 0)
		editor = av[2];
	else
		editor = get_fc_editor(state);
	first = resolve_idx(state, (ac > 1 && av[1][0] != '-') ? av[1] : NULL);
	last = resolve_idx(state, (ac > 2 && av[2][0] != '-') ? av[2] : NULL);
	if (first == last)
		last = first;
	return (fc_edit_run(state, editor, first, last));
}
