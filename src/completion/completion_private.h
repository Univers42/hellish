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

#ifndef COMPLETION_PRIVATE_H
# define COMPLETION_PRIVATE_H

# include "libft.h"
# include <stddef.h>
# include <dirent.h>

extern char		*g_builtins[];
extern int		g_cmd_idx;
extern char		*g_path_dirs_cache;

void	free_split(char **arr);
void	cmd_gen_cleanup(char ***path_dirs);
void	cmd_gen_init(char ***path_dirs, int *dir_idx);
char	*cmd_gen_scan_dir(DIR *d, const char *text, size_t tlen);
char	*cmd_gen_dirs(char ***path_dirs, int *dir_idx, size_t tlen,
			const char *text);

#endif
