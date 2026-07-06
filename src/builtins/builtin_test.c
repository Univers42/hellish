/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_test.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <sys/stat.h>

int		test_file_op(const char *op, const char *path);
int		test_str_op(const char *op, const char *s1, const char *s2);
int		test_int_op(const char *op, const char *s1, const char *s2);

/* All test functions follow the POSIX convention: 0 = true, 1 = false,
   2 = error. This is the *inverse* of C's boolean, which trips everyone up
   at least once. Keep it in mind when reading the returns below. */

/* Leaf evaluator for `-X operand`: -z/-n (string length), any other
   single-letter -X flag is a file test. An unknown operator is an error
   (2), matching bash's "unary operator expected". */
int	tx_test_unary(char **a)
{
	if (ft_strcmp(a[0], "-z") == 0)
		return (ft_strlen(a[1]) != 0);
	if (ft_strcmp(a[0], "-n") == 0)
		return (ft_strlen(a[1]) == 0);
	if (a[0][0] == '-' && a[0][1] && !a[0][2])
		return (test_file_op(a[0], a[1]));
	return (ft_eprintf("test: %s: unary operator expected\n", a[0]), 2);
}

/* Leaf evaluator for `a op b`; only called when op is a known binary
   operator (tx_is_binop), so the routing is exhaustive: string ops first,
   everything else is one of the -eq family. */
int	tx_test_binary(char **a)
{
	if (ft_strcmp(a[1], "=") == 0
		|| ft_strcmp(a[1], "==") == 0
		|| ft_strcmp(a[1], "!=") == 0)
		return (test_str_op(a[1], a[0], a[2]));
	return (test_int_op(a[1], a[0], a[2]));
}

/* POSIX two-argument rule: `! s` negates the non-empty-string test,
   otherwise the first word must be a unary operator. */
static int	eval_two(char **av)
{
	if (ft_strcmp(av[0], "!") == 0)
		return (ft_strlen(av[1]) != 0);
	return (tx_test_unary(av));
}

/* Entry point for the test evaluator, following bash's posixtest() layout:
   fixed rules for 0-2 arguments, the binary fast path for exactly `a op b`,
   and the full -a/-o/!/( ) expression grammar (builtin_test_expr.c) for
   everything else. A parse that errors or leaves tokens behind exits 2. */
int	eval_test(char **av, int ac)
{
	t_tx	t;
	int		r;

	if (ac == 0)
		return (1);
	if (ac == 1)
		return (ft_strlen(av[0]) == 0);
	if (ac == 2)
		return (eval_two(av));
	if (ac == 3 && tx_is_binop(av[1]))
		return (tx_test_binary(av));
	t = (t_tx){av, ac, 0, 0};
	r = tx_or(&t);
	if (r == 2 || t.err)
		return (2);
	if (t.i != ac)
		return (ft_eprintf("test: syntax error\n"), 2);
	return (r);
}

/* `[`, `[[`, and `test`. `[[` runs the logical evaluator (`&&`/`||`/`!`/`( )`);
   `[` and `test` run the flat single-test evaluator. eval_bracketed validates
   the matching close bracket (`]`/`]]`) and strips it. */
int	builtin_test(t_shell *state, t_vec argv)
{
	char	**av;
	int		ac;

	av = (char **)argv.ctx;
	ac = (int)argv.len;
	if (ac > 0 && av[0][0] == '[' && av[0][1] == '[')
		return (eval_bracketed(state, av, ac, 1));
	if (ac > 0 && ft_strcmp(av[0], "[") == 0)
		return (eval_bracketed(state, av, ac, 0));
	return (eval_test(av + 1, ac - 1));
}
