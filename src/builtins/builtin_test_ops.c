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

static int	test_mode_flag(const char *op, struct stat *st)
{
	if (ft_strcmp(op, "-g") == 0)
	{
		if (st->st_mode & S_ISGID)
			return (0);
		return (1);
	}
	if (ft_strcmp(op, "-s") == 0)
	{
		if (st->st_size > 0)
			return (0);
		return (1);
	}
	if (ft_strcmp(op, "-u") == 0)
	{
		if (st->st_mode & S_ISUID)
			return (0);
		return (1);
	}
	if (ft_strcmp(op, "-S") == 0)
	{
		if (S_ISSOCK(st->st_mode))
			return (0);
		return (1);
	}
	return (1);
}

static int	test_file_type(const char *op, struct stat *st)
{
	if (ft_strcmp(op, "-b") == 0)
		return (!S_ISBLK(st->st_mode));
	if (ft_strcmp(op, "-c") == 0)
		return (!S_ISCHR(st->st_mode));
	if (ft_strcmp(op, "-d") == 0)
		return (!S_ISDIR(st->st_mode));
	if (ft_strcmp(op, "-f") == 0)
		return (!S_ISREG(st->st_mode));
	if (ft_strcmp(op, "-p") == 0)
		return (!S_ISFIFO(st->st_mode));
	return (test_mode_flag(op, st));
}

static int	test_file_access(const char *op, const char *path)
{
	if (ft_strcmp(op, "-r") == 0)
		return (access(path, R_OK) != 0);
	if (ft_strcmp(op, "-w") == 0)
		return (access(path, W_OK) != 0);
	if (ft_strcmp(op, "-x") == 0)
		return (access(path, X_OK) != 0);
	return (1);
}

int	test_file_op(const char *op, const char *path)
{
	struct stat	st;

	if (ft_strcmp(op, "-e") == 0)
		return (stat(path, &st) != 0);
	if (ft_strcmp(op, "-L") == 0 || ft_strcmp(op, "-h") == 0)
	{
		if (lstat(path, &st) == 0 && S_ISLNK(st.st_mode))
			return (0);
		return (1);
	}
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
		return (ft_strcmp(s1, s2) != 0);
	if (ft_strcmp(op, "!=") == 0)
		return (ft_strcmp(s1, s2) == 0);
	return (1);
}
