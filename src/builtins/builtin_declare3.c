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
** -f prints the definition, rebuilt from the BODY's source text, captured at
** definition time by ast_source_text() (src/infrastructure/ast_span.c explains
** why recovering the span is exact and why a deparser would not be).
**
** The wrapper `name () { ... }` is synthesised rather than taken from source,
** because `{`, `}` and `()` are not AST tokens -- they carry no start pointer,
** so a span over the whole definition stops at the last WORD and comes back
** as `f() { echo hi;` with no closing brace. That does not re-parse, which
** makes it worse than useless: the one thing this output must do is survive
** a round trip through eval. Wrapping a body span cannot lose a delimiter.
**
** A function whose span could not be recovered -- one built by eval, where
** the buffer is gone -- says so instead of printing an invented body. */

/* bash prints one `declare -f NAME` line per function for a bare -F, and the
   bare name when specific names were asked for. */
static void	print_one(const char *name, bool bare)
{
	if (bare)
		ft_printf("%s\n", name);
	else
		ft_printf("declare -f %s\n", name);
}

int	list_all(t_shell *state)
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
int	list_named(t_shell *state, t_vec argv, size_t i)
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

/* -F: names only. */
int	declare_names(t_shell *state, t_vec argv, size_t i)
{
	if (i >= argv.len)
		return (list_all(state));
	return (list_named(state, argv, i));
}
