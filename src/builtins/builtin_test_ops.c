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

int	test_tty_op(const char *arg);

/* The file-test operators follow the 0=true/1=false convention of the whole
   test evaluator. The !S_IS…() / !(bit) pattern keeps each branch to one
   line: the macro returns non-zero when the type matches, so we invert to
   get 0 (true) when it matches and 1 (false) otherwise. */

/* Handle the less common mode flags: -g (setgid), -k (sticky bit), -s
   (non-empty), -u (setuid), -S (socket), -G (owned by the effective gid),
   -O (owned by the effective uid). The !(bit) inversion maps "bit set" to
   0 = true. Falls through to 1 (false) on anything unrecognised. */
static int	test_mode_flag(const char *op, struct stat *st)
{
	if (ft_strcmp(op, "-g") == 0)
		return (!(st->st_mode & S_ISGID));
	if (ft_strcmp(op, "-k") == 0)
		return (!(st->st_mode & S_ISVTX));
	if (ft_strcmp(op, "-s") == 0)
		return (!(st->st_size > 0));
	if (ft_strcmp(op, "-u") == 0)
		return (!(st->st_mode & S_ISUID));
	if (ft_strcmp(op, "-S") == 0)
		return (!S_ISSOCK(st->st_mode));
	if (ft_strcmp(op, "-G") == 0)
		return (st->st_gid != getegid());
	if (ft_strcmp(op, "-O") == 0)
		return (st->st_uid != geteuid());
	return (1);
}

/* File-type flags that need a successful stat() first: -b (block dev), -c
   (char dev), -d (dir), -f (regular file), -p (named pipe / FIFO). */
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

/* Permission checks via access(2): -r readable, -w writable, -x executable.
   access() uses the real UID/GID, not effective — fine for interactive use
   and consistent with what POSIX mandates for these operators. */
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

/* Top-level file-test dispatcher. -e just needs stat() to succeed, and -a
   in unary position is its historical XSI alias (bash accepts both). -L/-h
   use lstat() so they test the symlink itself, not what it points to. The
   access checks are handled before stat() so we do not need a struct stat
   for them. Everything else calls stat() first and returns 1 (false) on
   error, which is correct: a file that cannot be stat'd is not a regular
   file, not a directory, etc. */
int	test_file_op(const char *op, const char *path)
{
	struct stat	st;

	if (ft_strcmp(op, "-e") == 0 || ft_strcmp(op, "-a") == 0)
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
	if (ft_strcmp(op, "-t") == 0)
		return (test_tty_op(path));
	if (stat(path, &st) != 0)
		return (1);
	return (test_file_type(op, &st));
}

/* String comparison operators. We accept both `=` (POSIX) and `==` (bash
   extension) for equality, and `<` / `>` for lexicographic order — both
   bash's test and dash treat them as plain strcmp, no locale collation.
   Return 0 (true) when the relation holds, 1 (false) otherwise. */
int	test_str_op(const char *op, const char *s1, const char *s2)
{
	if (ft_strcmp(op, "=") == 0 || ft_strcmp(op, "==") == 0)
		return (ft_strcmp(s1, s2) != 0);
	if (ft_strcmp(op, "!=") == 0)
		return (ft_strcmp(s1, s2) == 0);
	if (ft_strcmp(op, "<") == 0)
		return (!(ft_strcmp(s1, s2) < 0));
	if (ft_strcmp(op, ">") == 0)
		return (!(ft_strcmp(s1, s2) > 0));
	return (1);
}
