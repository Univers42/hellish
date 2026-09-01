/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_complete.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Declared here rather than in the private header: the 42 norm aligns
   every declaration in a file to one column, and `t_compspec` is too wide
   to reach the one that header already uses. */
t_compspec	*comp_find(t_shell *st, const char *name);

/* `complete` -- register what to offer when the user tabs an argument of a
** given command.
**
** Together with compgen this is #72 phase 4.  Both were absent, and the
** absence was quiet in the way that matters: `complete` exiting 127 as an
** unknown command meant every completion script aborted PART WAY through
** its own setup, leaving some functions defined and some not.  The file
** looked loaded and the shell was left half-configured.
**
** The spec is stored, printed back in reusable form, and consulted by the
** readline completer.  What is NOT acted on is bash's decoration set --
** `-o nospace`, `-X filter`, `-P/-S` -- which is recorded rather than
** rejected on purpose: a plugin passing `-o nospace` should register its
** completion and lose the spacing nicety, not fail to register at all.
*/

/* Find an existing spec for NAME, or NULL. */
t_compspec	*comp_find(t_shell *st, const char *name)
{
	size_t		i;
	t_compspec	*c;

	i = 0;
	while (i < st->compspecs.len)
	{
		c = (t_compspec *)vec_idx(&st->compspecs, i++);
		if (ft_strcmp(c->name, (char *)name) == 0)
			return (c);
	}
	return (NULL);
}

/* Replace the spec for NAME, or append a new one. Registering twice is how
   a completion script updates itself on reload, so the second call has to
   REPLACE rather than accumulate -- otherwise comp_find keeps answering
   with the first registration forever. Each name gets its own copies of
   the strings, since one `complete -W list a b` registers two specs. */
static void	comp_store(t_shell *st, const char *name, t_cmpopt *o)
{
	t_compspec	c;
	t_compspec	*old;

	c = (t_compspec){.name = NULL, .words = NULL, .func = NULL,
		.opts = NULL, .act = o->act};
	if (o->words)
		c.words = ft_strdup(o->words);
	if (o->func)
		c.func = ft_strdup(o->func);
	if (o->opts)
		c.opts = ft_strdup(o->opts);
	old = comp_find(st, name);
	if (old)
	{
		comp_free_spec(old);
		c.name = ft_strdup((char *)name);
		*old = c;
		return ;
	}
	c.name = ft_strdup((char *)name);
	comp_vec_push(st, &c);
}

/* `complete -p [NAME]`: print specs as commands that would recreate them.
   An unknown NAME is an error with status 1 -- scripts test exactly that
   to decide whether they still need to register. */
static int	comp_print(t_shell *st, t_vec argv, size_t i)
{
	t_compspec	*c;

	if (i >= argv.len)
		return (comp_print_all(st), 0);
	while (i < argv.len)
	{
		c = comp_find(st, ((char **)argv.ctx)[i]);
		if (!c)
			return (ft_eprintf("%s: complete: %s: no completion "
					"specification\n", st->ctx, ((char **)argv.ctx)[i]), 1);
		comp_print_one(c);
		i++;
	}
	return (0);
}

/* complete [-abcdfkv] [-A action] [-W list] [-F func] [-o opt] [-pr] NAME…
   With no NAME and no -p it prints every spec, like bash — EXCEPT when a
   default-spec selector (-D/-E/-I) was given: `complete -D -F loader` is
   how bash-completion installs its lazy loader, names none, and bash sets
   the default spec silently. Falling into list mode there dumped every
   registration to stdout at the end of `. bash_completion` (#105). */
int	builtin_complete(t_shell *state, t_vec argv)
{
	t_cmpopt	o;
	size_t		i;

	o = (t_cmpopt){0};
	i = comp_parse_opts(state, argv, &o);
	if (i == CG_OPT_ERR)
		return (2);
	if (o.print)
		return (comp_print(state, argv, i));
	if (o.remove)
		return (comp_remove(state, argv, i));
	if (i >= argv.len && o.defsel)
		return (0);
	if (i >= argv.len)
		return (comp_print_all(state), 0);
	while (i < argv.len)
		comp_store(state, ((char **)argv.ctx)[i++], &o);
	return (0);
}
