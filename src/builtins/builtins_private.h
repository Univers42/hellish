/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtins_private.h                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:02:26 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 15:00:57 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BUILTINS_PRIVATE_H
# define BUILTINS_PRIVATE_H

# include "shell.h"
# include "env.h"
# include "libft.h"
# include "ft_builtins.h"
# include <stdlib.h>
# include "helpers.h"
# include "input.h"

# define CD_ERROR "cd: error retrieving current directory: getcwd: \
				cannot access parent directories: \
				No such file or directory\n"
# define PWD_ERR_CUR_DIR "pwd: error retrieving current directory: \
				getcwd: cannot access parent directories: \
				No such file or directory\n"
# define OLDPWD_NO_SET "%s: cd: OLDPWD not set\n"
# define OLDPWD_NAME "OLDPWD"
# define PWD_NAME "PWD"

void	try_unset(t_shell *state, char *key);
int		builtin_unset(t_shell *state, t_vec argv);
int		parse_flags(t_vec argv, int *n, int *e);
int		print_args(int e, t_vec argv, size_t i);
int		builtin_echo(t_shell *state, t_vec argv);
int		builtin_env(t_shell *state, t_vec argv);
void	exit_clean(t_shell *state, int code);
int		builtin_exit(t_shell *state, t_vec argv);
void	parse_export_arg(char *str, char **ident, char **val);
char	strip_surrounding_quotes(char **val);
void	consume_following_value(t_vec av, int *i, char **val);
int		handle_identifier(t_shell *st, char *id, char *val, const char *argv0);
int		process_arg(t_shell *st, t_vec av, int *ip);
void	collect_and_print_exported(t_shell *st);
int		builtin_export(t_shell *st, t_vec av);
int		cd_home(int *e, t_shell *state);
int		builtin_pwd(t_shell *state, t_vec argv);
void	try_unset(t_shell *state, char *key);
int		builtin_unset(t_shell *state, t_vec argv);

void	print_exit_if_readline(t_shell *state);
int		handle_no_args(t_shell *state, t_vec argv);
size_t	handle_double_dash(t_shell *state, t_vec argv, size_t i);
int		handle_non_numeric(t_shell *state, t_vec argv, size_t i, int *ret);
char	*expand_export_value(t_shell *st, char *val, bool allow_expand);
char	strip_surrounding_quotes(char **val);
bool	ft_is_valid_ident(char *id);
void	parse_export_arg(char *str, char **ident, char **val);

void	update_pwd_vars(t_shell *state);
bool	is_redir_operator(char *s);
int		count_real_args(t_vec argv);
char	*get_first_real_arg(t_vec argv);
int		check_args(t_vec argv);
int		cd_do_chdir(t_shell *state, t_vec argv, int *e);
void	cd_refresh_cwd(t_shell *state, t_vec argv, char *cwd);
int		handle_too_many_args(t_shell *state, t_vec argv, size_t i);

int		parse_redir_len(const char *arg);
bool	redir_needs_next(const char *arg);

#endif