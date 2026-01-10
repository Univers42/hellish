/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtins.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:25 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:25 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BUILTINS_H
# define BUILTINS_H

# include "../libft/libft.h"
# include "../shell.h"
# include <errno.h>
# include <stdbool.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

int		mini_atoi_base(char **str, int base, int len);
int		cd_home(int *e, t_state *state);
void	parse_numeric_escape(char **str);
int		e_parser(char *str);
int		parse_flags(t_vec argv, int *n, int *e);
int		print_args(int e, t_vec argv, size_t i);
void	try_unset(t_state *state, char *key);

int		builtin_echo(t_state *state, t_vec argv);
int		builtin_pwd(t_state *state, t_vec argv);
int		builtin_exit(t_state *state, t_vec argv);
int		builtin_cd(t_state *state, t_vec argv);
int		builtin_env(t_state *state, t_vec argv);
int		builtin_export(t_state *state, t_vec argv);
int		builtin_unset(t_state *state, t_vec argv);

int	(*builtin_func(char *name))(t_state *state, t_vec argv);

#endif