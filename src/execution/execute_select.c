/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_select.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "../builtins/builtins_private.h"
#include "env.h"

int	handle_loop_ctl(t_shell *state);

/* select NAME [in words]; do list; done -- bash's menu loop, issue #122.
**
** The words are numbered on stderr, PS3 (default "#? ") is printed, and
** `read` fills REPLY.  A reply that is a number in range binds NAME to
** that word; anything else binds it to the empty string; an empty line
** shows the menu again without running the body.  After the body the
** menu is printed again only if REPLY is empty -- ksh's rule, which bash
** is built with by default.  EOF ends the loop with status 1 and, as bash
** does, a newline on STDOUT.  Only break, return and EOF leave the loop.
*/

/* One prompt/read round.  bash calls its own `read` builtin here, so the
   backslash handling, the EOF rule and where REPLY lands are read's; the
   same call gives the same answers. */
static bool	select_ask(t_shell *state, t_vec *words, bool menu)
{
	char	*av[2];
	t_vec	argv;
	char	*ps3;

	if (menu)
		select_print_menu(state, words);
	ps3 = env_expand(state, "PS3");
	if (!ps3)
		ps3 = "#? ";
	sel_write(STDERR_FILENO, ps3, ft_strlen(ps3));
	av[0] = "read";
	av[1] = NULL;
	argv = (t_vec){.cap = 2, .len = 1, .elem_size = sizeof(char *),
		.ctx = av};
	if (builtin_read(state, argv) != 0)
		return (sel_write(STDOUT_FILENO, "\n", 1), false);
	return (true);
}

/* The word REPLY names, or "" for an empty or out-of-range choice -- bash
   binds NAME to the empty string rather than leaving it alone. */
static char	*select_pick(t_shell *state, t_vec *words)
{
	char	*reply;
	long	n;

	reply = env_expand(state, "REPLY");
	if (!reply || !sel_number(reply, &n))
		return ("");
	if (n < 1 || n > (long)words->len)
		return ("");
	return (((char **)words->ctx)[n - 1]);
}

/* One iteration: ask, and unless the line was empty (menu again, no body)
   bind NAME and run the body.  False when the loop is over: EOF (status
   1, like bash) or a break / return the body asked for. */
static bool	select_round(t_shell *state, t_select *s)
{
	char	*reply;

	if (!select_ask(state, &s->words, s->menu))
		return (s->status = res_status(1), false);
	reply = env_expand(state, "REPLY");
	s->menu = (!reply || !*reply);
	if (s->menu)
		return (true);
	set_for_var(state, s->name, select_pick(state, &s->words));
	s->status = execute_tree_node(state, &s->body);
	if (handle_loop_ctl(state))
		return (false);
	reply = env_expand(state, "REPLY");
	s->menu = (!reply || !*reply);
	return (true);
}

/* The words to choose from: "$@" when there is no `in` clause, else the
   expanded list.  Returns whether the positional snapshot was taken, so
   the matching release is called. */
static bool	select_words(t_shell *state, t_executable_node *exe, t_select *s)
{
	size_t	wc;

	wc = exe->node->children.len - 1;
	if (wc == 0 && !exe->node->negate)
		return (snapshot_positionals(state, &s->words), true);
	s->words = for_expand_words(state, exe->node, wc);
	return (false);
}

/* select NAME [in wordlist]; do body; done.  Same AST shape as for: the
   name in the node's token, children = words then body.  An empty list
   runs nothing and succeeds, as in bash. */
t_execution_state	execute_select(t_shell *state, t_executable_node *exe)
{
	t_select	s;
	bool		pos;
	bool		run;

	ft_assert(exe->node->children.len >= 1);
	pos = select_words(state, exe, &s);
	s.name = ft_strndup(exe->node->token.start, exe->node->token.len);
	s.body = create_exe_node(STDIN_FILENO, STDOUT_FILENO,
			vec_idx(&exe->node->children, exe->node->children.len - 1), true);
	s.status = res_status(0);
	s.menu = true;
	fire_debug_trap(state);
	run = (s.words.len > 0);
	state->loop_depth++;
	while (run)
		run = select_round(state, &s);
	state->loop_depth--;
	if (pos)
		free_positional_snapshot(&s.words);
	else
		free_word_vec(&s.words);
	return (xfree(s.name), s.status);
}
