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

/* Cap on history entries kept in memory and on disk (like bash HISTFILESIZE).
   Bounds startup load time and the per-line readline/fork footprint. */
# ifndef HIST_MAX
#  define HIST_MAX 2000
# endif

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
void		add_history_line(const char *cmd);
void		parse_history_file(t_shell *state);
t_string	encode_cmd_hist(char *cmd);
void		manage_history(t_shell *state);
bool		worthy_of_being_remembered(t_shell *state);
void		init_history(t_shell *state);
void		free_hist(t_shell *state);
char		*get_hist_file_path(t_shell *state);
char		*hist_entry_at(t_shell *state, int idx);
char		*hist_last(t_shell *state);
char		*hist_search_prefix(t_shell *state, const char *prefix, int len);
char		*hist_search_contains(t_shell *state, const char *needle, int len);
char		*resolve_bang(t_shell *state, const char *s, size_t *adv);
int			in_sq(const char *s, size_t pos);
char		*replace_first(const char *s, const char *old, const char *nw);
char		*quick_sub(t_shell *state, const char *input);

#endif