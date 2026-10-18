/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_rl.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 03:20:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 03:20:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "history_private.h"
#include <readline/readline.h>
#include <stdlib.h>

/* Dropping entries from readline's history list, without dropping what
** readline hung on them.
**
** Edit a recalled line and move off it with an arrow, and readline keeps
** the edit: the entry's text becomes the edited one and its `data` becomes
** the line's undo list, so coming back can still undo to the original.
** remove_history() and free_history_entry() hand that data back to the
** caller, and clear_history() admits in a comment that it "loses because
** we cannot free the data". hellish ignored it everywhere.
**
** Invisible while readline ran in a forked child: the child's history was
** thrown away with the child. In-process it is the shell's own heap, so
** HISTCONTROL=erasedups, the HISTSIZE cap, `history -d` and `history -c`
** each leaked an undo list -- and with ASan every forked subshell after
** that reported it at exit, once per `$(...)` a theme ran.
**
** The list being edited right now is readline's (rl_undo_list), and a zle
** widget can run `history -d` while it is: that one is left for readline,
** which frees it when the line ends. Memory from readline's xmalloc is
** libc's, so it goes back through free(), never xfree(). */

/* Free one undo list, unless readline is still using it. */
static void	undo_list_free(UNDO_LIST *u)
{
	UNDO_LIST	*next;

	if (u == rl_undo_list)
		return ;
	while (u)
	{
		next = u->next;
		free(u->text);
		free(u);
		u = next;
	}
}

/* remove_history(idx), and everything the entry owned. */
void	hist_rl_drop(int idx)
{
	HIST_ENTRY	*e;

	e = remove_history(idx);
	if (e)
		undo_list_free((UNDO_LIST *)free_history_entry(e));
}

/* Before each read: free the undo list readline parked with its saved
** copy of the typed line.
**
** Walking the history from a typed line (previous-history: Ctrl-P, or Up
** with prefix search off) makes readline save that line, with its undo
** list as the entry's data, so Down can bring it back. When the line ends
** elsewhere -- Enter or ^C on a recalled entry -- the saved copy outlives
** the read, and the next readline() frees it without the undo list:
** readline 8.1 (Ubuntu 22.04's) leaked the typed text's undo records once
** per such line, which ASan reported at exit.
**   rl_maybe_unsave_line() is the public way to take that list back into
** rl_undo_list, and rl_free_undo_list() frees it as readline does, also
** dropping any history entry still pointing at it. Unsave rings the bell
** when nothing is saved, so the save before it makes sure something is.
** Both rewrite rl_line_buffer, which is harmless here: the read that
** follows starts from an empty line. */
void	hist_rl_reclaim_saved(void)
{
	if (!rl_line_buffer)
		return ;
	rl_maybe_save_line();
	rl_maybe_unsave_line();
	rl_free_undo_list();
}

/* clear_history(), freeing the undo lists it cannot. */
void	hist_rl_clear(void)
{
	HIST_ENTRY	**list;
	int			i;

	list = history_list();
	i = 0;
	while (list && list[i])
	{
		undo_list_free((UNDO_LIST *)list[i]->data);
		list[i]->data = NULL;
		i++;
	}
	clear_history();
}
