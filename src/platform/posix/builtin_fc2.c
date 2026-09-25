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

/* Fork the editor on `tmpf`, wait for it, then if it exited successfully
   read the (possibly modified) file back into state->input so the REPL
   re-executes it on the next iteration. Always unlinks the temp file. */
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

/* Resolve the editor: $FCEDIT first (fc-specific), then $EDITOR (general),
   then fall back to /bin/ed (POSIX mandates a default editor for fc). */
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

/* fc [-e editor] [first [last]]: write entries to a temp file, open the
   editor, then queue the result for re-execution. No operand edits the
   previous command -- the fc line itself is out of fc's view (fc_total). */
static int	fc_run(t_shell *state, t_fcopt *o)
{
	int			total;
	int			first;
	int			last;
	const char	*editor;

	editor = o->editor;
	if (!editor)
		editor = get_fc_editor(state);
	total = fc_total(state);
	if (fc_resolve_idx(state, NULL, total, &first)
		|| (o->nops > 0 && fc_resolve_idx(state, o->ops[0], total, &first)))
		return (1);
	last = first;
	if (o->nops > 1 && fc_resolve_idx(state, o->ops[1], total, &last))
		return (1);
	if (first > last)
		return (fc_edit_run(state, editor, last, first));
	return (fc_edit_run(state, editor, first, last));
}

/* fc [-e editor] [-lnr] [first [last]]: list the history (-l) or edit and
   re-run part of it. Options cluster (fc_parse). `-s` and `-e -` -- re-run
   without an editor -- are refused OUT LOUD rather than falling into edit
   mode, which is what every unrecognised spelling used to do: an editor
   started where the caller expected output, as in `$(fc -ln -1)`. */
int	builtin_fc(t_shell *state, t_vec argv)
{
	t_fcopt	o;
	int		st;

	st = fc_parse(state, (char **)argv.ctx, (int)argv.len, &o);
	if (st)
		return (st);
	if (o.list && !state->hist.hist_active)
		return (0);
	if (o.list)
		return (fc_list(state, &o));
	if (o.subst)
		return (ft_eprintf("%s: fc: -s: not supported\n", state->ctx), 2);
	if (!state->hist.hist_active || fc_total(state) <= 0)
		return (ft_eprintf("%s: fc: no command history\n", state->ctx), 1);
	return (fc_run(state, &o));
}
