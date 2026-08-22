/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   completion_private.h                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Shared state and helpers for the command-completion generator.
   The generator is stateful (readline calls it repeatedly until it
   returns NULL), so one t_cmd_gen has to survive between those calls.
   It lives as a single function-local static in cmd_generator and is
   reset by cmd_gen_init at the start of each new completion sequence. */

#ifndef COMPLETION_PRIVATE_H
# define COMPLETION_PRIVATE_H

# include "libft.h"
# include <stddef.h>
# include <dirent.h>

/* One TAB press worth of command-generator state: PATH split into dirs
   (cache owns the bytes they point into), how far the builtin list and
   the dir list have been walked, and the directory currently open. */
typedef struct s_cmd_gen
{
	char	**dirs;
	char	*cache;
	int		bidx;
	int		idx;
	DIR		*dh;
}	t_cmd_gen;

/* NULL-terminated list of built-in names, checked before the PATH scan */
extern char		*g_builtins[];

void	free_split(char **arr);
void	cmd_gen_cleanup(t_cmd_gen *g);
void	cmd_gen_init(t_cmd_gen *g);
char	*cmd_gen_scan_dir(DIR *d, const char *text, size_t tlen);
char	*cmd_gen_dirs(t_cmd_gen *g, size_t tlen, const char *text);
char	*rl_dup(const char *s);
char	*rl_dup_dollar(const char *name, size_t len);

#endif
