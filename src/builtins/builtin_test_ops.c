/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_test_ops.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <sys/stat.h>

static int	test_file_type(const char *op, struct stat *st)
{
	if (ft_strcmp(op, "-b") == 0)
		return (S_ISBLK(st->st_mode) ? 0 : 1);
	if (ft_strcmp(op, "-c") == 0)
		return (S_ISCHR(st->st_mode) ? 0 : 1);
	if (ft_strcmp(op, "-d") == 0)
		return (S_ISDIR(st->st_mode) ? 0 : 1);
	if (ft_strcmp(op, "-f") == 0)
		return (S_ISREG(st->st_mode) ? 0 : 1);
	if (ft_strcmp(op, "-g") == 0)
		return ((st->st_mode & S_ISGID) ? 0 : 1);
	if (ft_strcmp(op, "-p") == 0)
		return (S_ISFIFO(st->st_mode) ? 0 : 1);
	if (ft_strcmp(op, "-s") == 0)
		return ((st->st_size > 0) ? 0 : 1);
	if (ft_strcmp(op, "-u") == 0)
		return ((st->st_mode & S_ISUID) ? 0 : 1);
	if (ft_strcmp(op, "-S") == 0)
		return (S_ISSOCK(st->st_mode) ? 0 : 1);
	return (1);
}

static int	test_file_access(const char *op, const char *path)
{
	if (ft_strcmp(op, "-r") == 0)
		return (access(path, R_OK) == 0 ? 0 : 1);
	if (ft_strcmp(op, "-w") == 0)
		return (access(path, W_OK) == 0 ? 0 : 1);
	if (ft_strcmp(op, "-x") == 0)
		return (access(path, X_OK) == 0 ? 0 : 1);
	return (1);
}

int	test_file_op(const char *op, const char *path)
{
	struct stat	st;

	if (ft_strcmp(op, "-e") == 0)
		return (stat(path, &st) == 0 ? 0 : 1);
	if (ft_strcmp(op, "-L") == 0 || ft_strcmp(op, "-h") == 0)
		return (lstat(path, &st) == 0 && S_ISLNK(st.st_mode) ? 0 : 1);
	if (ft_strcmp(op, "-r") == 0
		|| ft_strcmp(op, "-w") == 0
		|| ft_strcmp(op, "-x") == 0)
		return (test_file_access(op, path));
	if (stat(path, &st) != 0)
		return (1);
	return (test_file_type(op, &st));
}

int	test_str_op(const char *op, const char *s1, const char *s2)
{
	if (ft_strcmp(op, "=") == 0)
		return (ft_strcmp(s1, s2) == 0 ? 0 : 1);
	if (ft_strcmp(op, "!=") == 0)
		return (ft_strcmp(s1, s2) != 0 ? 0 : 1);
	return (1);
}

int	test_int_op(const char *op, const char *s1, const char *s2)
{
	long	a;
	long	b;

	a = ft_atoi(s1);
	b = ft_atoi(s2);
	if (ft_strcmp(op, "-eq") == 0)
		return (a == b ? 0 : 1);
	if (ft_strcmp(op, "-ne") == 0)
		return (a != b ? 0 : 1);
	if (ft_strcmp(op, "-gt") == 0)
		return (a > b ? 0 : 1);
	if (ft_strcmp(op, "-ge") == 0)
		return (a >= b ? 0 : 1);
	if (ft_strcmp(op, "-lt") == 0)
		return (a < b ? 0 : 1);
	if (ft_strcmp(op, "-le") == 0)
		return (a <= b ? 0 : 1);
	ft_eprintf("test: %s: unknown operator\n", op);
	return (2);
}
