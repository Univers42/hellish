/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtins_private.h                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:02:26 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/21 11:11:37 by dlesieur         ###   ########.fr       */
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
int		handle_identifier(t_shell *st, char *id, char *val, const char *argv0, const char *orig_arg);
int		process_arg(t_shell *st, t_vec av, int *ip);
void	collect_and_print_exported(t_shell *st);
int		builtin_export(t_shell *st, t_vec av);
int		cd_home(int *e, t_shell *state);
int		builtin_pwd(t_shell *state, t_vec argv);
void	try_unset(t_shell *state, char *key);
int		builtin_unset(t_shell *state, t_vec argv);

#endif