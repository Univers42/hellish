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

/* Palette slot ids for pal(): every colour the prompt uses, resolved at
   render time to a truecolor sequence (COLORTERM advertises 24-bit) or a
   256-colour fallback. Keep the two tables in prompt_palette.c in this
   exact order. */
enum e_pal
{
	PAL_BOX,
	PAL_USER,
	PAL_ROOT,
	PAL_HOST,
	PAL_CONN,
	PAL_CWDD,
	PAL_CWD,
	PAL_GIT,
	PAL_DIRTY,
	PAL_VENV,
	PAL_TIME,
	PAL_OK,
	PAL_ERR,
	PAL_JOBS,
	PAL_DUR,
	PAL_N
};

/* Prompt animation cells, filled by the parent in ps1_animated and read
   by the forked readline child's idle hook (plain fork inheritance, the
   same trick the old mascot used): one fully-rendered prompt string per
   frame, differing only in the \A glyph, plus the visible width of the
   last prompt row for the repaint math. count == 0 disables the idle
   hook entirely. The cells are SINGLE-SHOT: each ps1_animated render
   arms exactly one readline fork, and attach_input_readline disarms the
   parent right after forking -- a continuation read (dquote>, heredoc>)
   must never inherit frames describing a prompt block that is no longer
   the rows above its cursor. */
# define PANIM_FRAMES 8
# define PANIM_MAX 2048

typedef struct s_panim
{
	char	buf[PANIM_FRAMES][PANIM_MAX];
	int		count;
	int		last_w;
}	t_panim;

/* Cached "which repo (if any) owns the cwd" answer, keyed on the cwd
   string so a cd invalidates it. Turns the per-prompt O(depth) walk of
   parent directories into a single stat once the answer is known. */
typedef struct s_gitloc
{
	char	cwd[PATH_MAX];
	char	root[PATH_MAX];
	int		has;
	int		init;
}	t_gitloc;

void		vec_push_ansi(t_string *v, const char *seq);
int			get_cols(void);
int			measure_width(const char *str);
char		*shorten_path(const char *path, int maxlen);
const char	*pal(int id);
int			pal_truecolor(void);
t_gitloc	*repo_locate(void);
int			git_dirty_cached(const char *root);
char		*branch_for_dir(const char *dir);
void		push_user_seg(t_string *ret, t_prompt *p);
void		push_cwd_seg(t_string *ret, t_prompt *p);
char		*get_venv_name(void);
void		get_timebuf(char *buf, size_t buflen);
t_string	prompt_more_input(t_shell *state, t_parser *parser);
t_string	prompt_normal(t_shell *state);
t_string	ps1_render(t_shell *state, const char *fmt);
void		ps1_host(t_string *out, char kind);
void		ps1_dollar(t_shell *state, t_string *out, const char *f, int *i);
void		ps1_user(t_shell *state, t_string *out);
void		ps1_git(t_string *out);
void		ps1_status(t_shell *state, t_string *out);
void		ps1_duration(t_shell *state, t_string *out);
void		ps1_jobs(t_shell *state, t_string *out);
bool		ps1_escape_ext(t_shell *state, t_string *out, char c);
t_panim		*anim_cells(void);
void		ps1_anim(t_shell *state, t_string *out);
t_string	ps1_animated(t_shell *state, char *ps1);
int			visible_width_cstr(const char *s);
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