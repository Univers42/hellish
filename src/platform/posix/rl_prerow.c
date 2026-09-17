/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_prerow.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include <readline/readline.h>

/* The prompt's upper rows, written from inside readline instead of before
** it.
**
** split_prompt composes those rows in memory so they reach the tty in one
** write() -- that closed the per-BYTE window issues #10, #19 and #5 were
** filed about. It did not close the last one, because the remaining split
** is the kernel's rather than ours: a pty accepts only what fits in its
** buffer and reports a short count (11776 bytes, measured on a full one),
** so tty_write_all goes round again, and between those two chunks the line
** discipline is free to echo a byte the user typed. It lands inside a
** colour escape; every letter is a valid CSI final byte, so the sequence
** ends early and its tail prints as literal text -- `22;162;247m` alone on
** a line, which is what prompt_atomic_test.py's phase 2 reproduces.
**
** The fix is ORDER, not locking. readline calls rl_startup_hook from
** readline_internal_setup, which runs after rl_prep_terminal has already
** put the terminal in raw mode -- echo off. Writing the rows there costs
** nothing and leaves no window at all. Hushing echo around our own write
** worked too, and cost three ioctls and two sigactions per prompt:
** frontend_budget_test.py priced a bare Enter at 21 ioctls against a
** budget of 16, which is the gate that says a prompt must stay cheap. */

static t_string	*rp_cell(void)
{
	static t_string	rows;

	return (&rows);
}

/* readline is in raw mode now: put the rows out and disarm. */
static int	rp_emit(void)
{
	t_string	*f;

	f = rp_cell();
	if (f->len)
		tty_write_all(fileno(rl_outstream), (char *)f->ctx, f->len);
	xfree(f->ctx);
	f->ctx = NULL;
	f->len = 0;
	f->cap = 0;
	rl_startup_hook = NULL;
	return (0);
}

/* Take ownership of the composed rows and arm the hook. */
void	rl_prerow_arm(t_string *rows)
{
	t_string	*f;

	f = rp_cell();
	xfree(f->ctx);
	*f = *rows;
	rl_startup_hook = rp_emit;
}
