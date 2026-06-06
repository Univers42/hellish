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
   The generator is stateful (readline calls it repeatedly), so the
   globals below persist across calls within one TAB press.  They are
   reset by cmd_gen_init at the start of each new completion sequence. */

#ifndef COMPLETION_PRIVATE_H
# define COMPLETION_PRIVATE_H

# include "libft.h"
# include <stddef.h>
# include <dirent.h>

/* NULL-terminated list of built-in names, checked before the PATH scan */
extern char		*g_builtins[];
/* current position in g_builtins[] across repeated generator calls */
extern int		g_cmd_idx;
/* ft_strdup of PATH at completion start; split into path_dirs */
extern char		*g_path_dirs_cache;

void	free_split(char **arr);
void	cmd_gen_cleanup(char ***path_dirs);
void	cmd_gen_init(char ***path_dirs, int *dir_idx);
char	*cmd_gen_scan_dir(DIR *d, const char *text, size_t tlen);
char	*cmd_gen_dirs(char ***path_dirs, int *dir_idx, size_t tlen,
			const char *text);

#endif
