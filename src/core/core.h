/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 00:01:28 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 00:01:28 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CORE_H
# define CORE_H

# include "shell.h"
# include "shell.h"
# include "helpers.h"
# include "input.h"
# include "env.h"
# include <string.h>
# include "lexer.h"
# include <fcntl.h>
# include <errno.h>
# include <unistd.h>

uint32_t	select_mode_from_argv(char **argv);
void		init_arg(t_shell *state, char **argv);
void		init_file(t_shell *state, char **argv);
void		init_stdin_notty(t_shell *state);
void		init_cwd(t_shell *state);
void		ensure_essential_env_vars(t_shell *state);
void		handle_file_open_error(t_shell *state, char **argv);
void		read_file_to_buffer(int fd, t_shell *state);
void		update_context_from_file(t_shell *state, char **argv);
void		init_arg(t_shell *state, char **argv);
void		init_file(t_shell *state, char **argv);
void		init_stdin_notty(t_shell *state);
void		parse_debug_flag(const char *v, uint32_t *flags);
void		parse_single_flag(const char *arg, uint32_t *flags);
#endif
