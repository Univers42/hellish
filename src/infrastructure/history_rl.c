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
