/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_declare4.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 13:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/27 13:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "ft_builtins.h"

/* declare -f: print definitions. Split from builtin_declare3.c only
   because the 42 norm caps a file at five functions. */

/* -f: the definition itself. bash ends each with a newline; the captured
   span does not include one, so add it. */
static void	print_body(t_shell *state, t_shell_func *fn)
{
	if (fn->text)
		return ((void)ft_printf("%s ()\n{\n%s\n}\n", fn->name, fn->text));
	ft_eprintf("%s: declare: %s: source text unavailable "
		"(defined by eval?)\n", state->ctx, fn->name);
}

static int	bodies_of(t_shell *state, t_vec argv, size_t i)
{
	t_shell_func	*arr;
	t_shell_func	*fn;
	size_t			n;
	int				missing;

	if (i >= argv.len)
	{
		arr = (t_shell_func *)state->functions.ctx;
		n = 0;
		while (n < state->functions.len)
			print_body(state, &arr[n++]);
		return (0);
	}
	missing = 0;
	while (i < argv.len)
	{
		fn = func_lookup(state, ((char **)argv.ctx)[i++]);
		if (fn)
			print_body(state, fn);
		else
			missing = 1;
	}
	return (missing);
}

int	declare_functions(t_shell *state, t_vec argv, size_t i, bool bodies)
{
	if (i < argv.len && ft_strcmp(((char **)argv.ctx)[i], "--") == 0)
		i++;
	if (bodies)
		return (bodies_of(state, argv, i));
	return (declare_names(state, argv, i));
}

/* `declare -ft name` (or -fx, -fr, -fg): -f combined with an ATTRIBUTE
   letter is bash's "apply the attribute to these functions" -- it prints
   NOTHING and answers 0 when every name is a function, 1 otherwise
   (measured on the oracle). It used to fall into the print path, so
   bash-preexec's closing `declare -ft __bp_install ...` dumped both
   function bodies onto the screen of every fresh install (issue #88).
   The attributes themselves are accepted and dropped: trace/export
   semantics for functions are not implemented, and silently succeeding
   is what bash's own scripts expect from the call. */
int	declare_func_attrs(t_shell *state, t_vec argv, size_t i)
{
	int	rc;

	rc = 0;
	while (i < argv.len)
	{
		if (!func_lookup(state, ((char **)argv.ctx)[i]))
			rc = 1;
		i++;
	}
	return (rc);
}
