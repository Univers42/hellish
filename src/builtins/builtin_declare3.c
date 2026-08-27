/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_declare3.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 13:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/27 13:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "ft_builtins.h"

/* declare -F / declare -f: what did this configuration actually define?
**
** Both returned NOTHING, always, and exited 0 while doing it -- declare_scan()
** knows only p/x/A/n/i, so -F and -f were swallowed as no-op option words and
** the argument loop ran zero times. Issue #71 item 2: without introspection a
** plugin manager cannot build a `help` command, cannot show what a plugin
** gave you, and cannot detect two plugins both defining `gs`. The reporter had
** to make every module hand-register its own aliases and functions into a
** registry maintained by hand, so a plugin that forgets to register is
** invisible.
**
** -F is exact and cheap: state->functions is already an iterable vec.
**
** -f prints the definition's SOURCE TEXT, captured at definition time by
** ast_source_text() (see src/infrastructure/ast_span.c for why that is exact
** and why a deparser would not be). A function whose span could not be
** recovered -- one built by eval, where the buffer is gone -- says so instead
** of printing an invented body.
**
** Divergence from bash: bash re-indents, so `f() { echo hi; }` comes back as
** four pretty-printed lines. This gives back what was written. Both
** round-trip through eval; for reading someone else's plugin, the original
** layout is the more useful answer. */

/* bash prints one `declare -f NAME` line per function for a bare -F, and the
   bare name when specific names were asked for. */
static void	print_one(const char *name, bool bare)
{
	if (bare)
		ft_printf("%s\n", name);
	else
		ft_printf("declare -f %s\n", name);
}

static int	list_all(t_shell *state)
{
	t_shell_func	*arr;
	size_t			i;

	arr = (t_shell_func *)state->functions.ctx;
	i = 0;
	while (i < state->functions.len)
		print_one(arr[i++].name, false);
	return (0);
}

/* With operands: print each name that IS a function, and report 1 if any
   named operand was not -- the status bash scripts branch on. */
static int	list_named(t_shell *state, t_vec argv, size_t i)
{
	int		missing;
	char	*name;

	missing = 0;
	while (i < argv.len)
	{
		name = ((char **)argv.ctx)[i++];
		if (func_lookup(state, name))
			print_one(name, true);
		else
			missing = 1;
	}
	return (missing);
}

/* -f: the definition itself. bash ends each with a newline; the captured
   span does not include one, so add it. */
static void	print_body(t_shell *state, t_shell_func *fn)
{
	if (fn->text)
		return ((void)ft_printf("%s\n", fn->text));
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
	if (bodies)
		return (bodies_of(state, argv, i));
	if (i >= argv.len)
		return (list_all(state));
	return (list_named(state, argv, i));
}
