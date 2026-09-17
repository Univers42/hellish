/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_private.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:21:39 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:16:39 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Internal header for the buffered readline layer. readline is always called
   in a forked child (see rl.c: get_more_input_readline + bg_readline) because
   it installs global signal handlers and terminal state that would corrupt the
   parent shell's state if called directly. The child writes the line over a
   pipe and exits; the parent reads into a ring buffer (t_rl) and parcels it
   out one logical line at a time via buff_readline/return_new_line. */
#ifndef RL_PRIVATE_H
# define RL_PRIVATE_H

# include "shell.h"
# include <readline/readline.h>
# include <stdbool.h>
# include <signal.h>
# include "pal_wait.h"
# include <unistd.h>
# include "sh_input.h"
# include <stdio.h>
# include <wchar.h>
# include "helpers.h"
# include "prompt_private.h"

int		return_last_line(t_shell *state, t_string *ret);
int		return_new_line(t_shell *state, t_string *ret);
int		buff_readline(t_shell *state, t_string *ret, char *prompt);
void	buff_readline_update(t_rl *l);
void	buff_readline_reset(t_rl *l);
void	buff_readline_init(t_rl *ret);
void	update_ctx(t_shell *state);
int		get_more_input_notty(t_shell *state);
int		visible_width_cstr(const char *s);
void	rl_preinit(t_rl *l);

/* The line reader (src/platform/posix/rl_getc.c, rl_idle.c, rl_sig.c). */
# ifndef READERR
#  define READERR -2
# endif
# define RL_AGAIN -3

int		rl_getc_hook(FILE *stream);
int		rl_idle_timeout(void);
int		rl_idle_fd(void);
void	rl_idle_event(int r);
void	rl_block(sigset_t *old);
int		*rl_intr_cell(void);
int		*rl_editor_abort_cell(void);
bool	rl_should_abort(void);
int		rl_abort_value(void);
void	rl_abort_reset(void);

#endif