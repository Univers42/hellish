/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_private.h                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:20:43 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 17:22:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HISTORY_PRIVATE_H
# define HISTORY_PRIVATE_H

# include "shell.h"
# include <fcntl.h>
# include <stdio.h>
# include <readline/history.h>
# include <stdbool.h>
# include <unistd.h>
# include "helpers.h"
# include "env.h"

t_string	parse_single_cmd(t_string hist, size_t *cur);
t_vec		parse_hist_file(t_string hist);
void		parse_history_file(t_shell *state);
t_string	encode_cmd_hist(char *cmd);
void		manage_history(t_shell *state);
bool		worthy_of_being_remembered(t_shell *state);
void		init_history(t_shell *state);
void		free_hist(t_shell *state);
char		*get_hist_file_path(t_shell *state);

#endif