/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   progcomp.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 10:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 10:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "progcomp_private.h"
#include "completion_private.h"
#include "zle.h"
#include <readline/readline.h>

/* `complete` specs, CONSULTED -- #72 phase 4, second half.
**
** The registry landed first: `complete -W 'add commit push' git` stored a
** spec, `complete -p` printed it back, `compgen` generated the same words on
** demand. None of it reached the line editor. The completer dispatched on
** what the WORD looked like -- '$' means variable, command position means
** command, everything else means "readline, do filenames" -- and never asked
** whether a spec existed for the command being typed.
**
** So the whole feature returned 0 and did nothing, which is the failure this
** project treats as the worst kind: a completion script loaded, registered,
** reported success, and TAB offered the contents of the current directory.
**
** THE STATE HANDLE. #72 records "the callback has no t_shell*" as a hard
** constraint, and it was true when it was written -- the completer read
** getenv() and environ directly. The ZLE work removed it: readline runs in a
** forked child and zle_install parks that child's t_shell where a callback
** can reach it. Same cell, same fork, same lifetime. A spec is therefore
** looked up in the real shell's table, and a -F function runs with the real
** shell's variables.
**
** THAT FORK IS ALSO THE BOUNDARY. A completion function that assigns a
** variable, or cds, changes the readline child and the parent never learns.
** For completion that is exactly right -- bash's own manual calls a
** completion function's side effects undefined -- and it is the same
** boundary a widget lives behind (src/platform/posix/zle_rl.c).
*/

t_vec	*pc_cell(void)
{
	static t_vec	v;

	if (v.elem_size == 0)
	{
		vec_init(&v);
		v.elem_size = sizeof(char *);
	}
	return (&v);
}

/* Empty the match list. Called before a build and again the moment readline
   has taken its copies, so nothing survives one TAB press. */
void	pc_reset(void)
{
	t_vec	*v;
	size_t	i;

	v = pc_cell();
	i = 0;
	while (i < v->len)
		xfree(((char **)v->ctx)[i++]);
	xfree(v->ctx);
	v->ctx = NULL;
	v->len = 0;
	v->cap = 0;
}

/* readline's generator protocol: state 0 starts a new sequence, then one
   match per call until NULL. Every string handed back is rl_dup'd, because
   readline frees them with libc free -- see rl_dup in completion.c. */
static char	*pc_generator(const char *text, int state)
{
	static size_t	i;
	t_vec			*v;

	(void)text;
	v = pc_cell();
	if (!state)
		i = 0;
	if (i >= v->len)
		return (NULL);
	return (rl_dup(((char **)v->ctx)[i++]));
}

/* Answer a TAB on an ARGUMENT word from the command's registered spec, or
** NULL to leave the existing dispatch alone.
**
** rl_attempted_completion_over is set as soon as a spec is FOUND, not once
** matches exist: bash does not fall back to filenames when a compspec
** produced nothing, and a shell that did would answer `git ch<TAB>` with the
** files in the current directory the moment a completion function declined.
** An empty answer from a spec is an answer.
*/
char	**progcomp_try(const char *text, int start, int end)
{
	t_shell		*st;
	t_compspec	*c;
	char		*cmd;
	char		**res;

	(void)end;
	st = *zle_state_cell();
	if (!st || !(st->shopt & SHOPT_PROGCOMP))
		return (NULL);
	cmd = pc_cmd_word(start);
	if (!cmd)
		return (NULL);
	c = comp_find(st, cmd);
	xfree(cmd);
	if (!c)
		return (NULL);
	rl_attempted_completion_over = 1;
	pc_build(st, c, text, start);
	res = rl_completion_matches(text, pc_generator);
	return (pc_reset(), res);
}
