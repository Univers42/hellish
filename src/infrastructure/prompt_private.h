/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_private.h                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:21:24 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 18:33:38 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PROMPT_PRIVATE_H
# define PROMPT_PRIVATE_H
# include "shell.h"
# include <stdio.h>
# include <readline/readline.h>
# include "parser.h"
# include <locale.h>
# include <sys/ioctl.h>
# include <unistd.h>
# include <limits.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>
# include <wchar.h>
# include <wctype.h>

typedef struct s_prompt
{
	char	*user;
	char	*short_cwd;
	char	*branch;
	char	*venv;
	int		cols;
	int		left_width;
	int		time_width;
	int		pad;
	int		branch_dirty;
	char	time_buf[32];
}	t_prompt;

void		vec_push_ansi(t_string *v, const char *seq);
int			get_cols(void);
int			measure_width(const char *str);
char		*shorten_path(const char *path, int maxlen);
char		*shorten_path(const char *path, int maxlen);
char		*get_venv_name(void);
void		get_timebuf(char *buf, size_t buflen);
t_string	prompt_more_input(t_parser *parser);
t_string	prompt_normal(void);
void		get_git_info(char **branch, int *dirty);
void		prompt_user_and_cwd(t_string *ret, t_prompt *p);
void		prompt_branch(t_string *ret, t_prompt *p);
void		prompt_venv(t_string *ret, t_prompt *p);
void		prompt_time_and_pad(t_string *ret, t_prompt *p);
void		prompt_branch_marker(t_string *ret, t_prompt *p);

#endif