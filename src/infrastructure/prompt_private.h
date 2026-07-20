/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_private.h                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:21:24 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 12:43:36 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Internal header for the prompt subsystem. Shared by prompt.c, prompt_utils*.c
   prompt_metadata*.c and the mascot files. The t_prompt struct collects all the
   metadata gathered during one render pass so each segment function can both
   contribute text to `ret` and update p->vis_w (accumulated visible width)
   for the padding calculation at the end. */
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
	int		vis_w;
	int		left_width;
	int		time_width;
	int		pad;
	int		branch_dirty;
	int		exit_status;
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
t_string	prompt_normal(t_shell *state);
void		get_git_info(char **branch, int *dirty);
void		prompt_user_and_cwd(t_string *ret, t_prompt *p);
void		prompt_branch(t_string *ret, t_prompt *p);
void		prompt_venv(t_string *ret, t_prompt *p);
void		prompt_time_and_pad(t_string *ret, t_prompt *p);

/* the blinking devil mascot + its readline-idle animation */
int			push_mascot(t_string *ret, size_t frame, int status);
void		render_prompt(t_string *ret, size_t frame, int status);
size_t		*anim_frame(void);
long long	*anim_dur_ms(void);
int			*anim_jobs(void);
const char	*user_color(void);
void		render_extras(t_string *ret, t_prompt *p);
void		prompt_arrow_row(t_string *ret, t_prompt *p);
int			*anim_status(void);
int			mascot_hook(void);
void		mascot_install(void);
void		redraw_mascot(t_string *r);

#endif