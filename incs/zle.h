/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zle.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 15:45:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 15:45:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* zsh's line editor, on readline. The registry and the bindings are in
   src/editing/; the readline half is in src/platform/posix/zle_rl*.c,
   because every one of those functions is a single readline call.

   WHAT IS NOT HERE, and is reported rather than accepted: region_highlight,
   the (start, end, style) array zsh repaints from. readline has no styled
   region model, and it is what zsh-syntax-highlighting is built on. */

#ifndef ZLE_H
# define ZLE_H

# include "libft.h"

typedef struct s_shell	t_shell;

/* One registered widget: the name a plugin binds, and the shell function
   that runs when the key fires. */
typedef struct s_zle_widget
{
	char	*name;
	char	*fn;
}	t_zle_widget;

t_vec			*zle_widgets(void);
void			zle_widget_add(const char *name, const char *fn);
t_zle_widget	*zle_widget_get(const char *name);
void			zle_widgets_free(void);

/* One recorded key binding. `seq` is what the plugin wrote (zsh escape
   syntax); `raw` is the bytes readline reports at dispatch time, filled in
   at install. The two differ and the difference is silent -- see
   zle_bind_raw. */
typedef struct s_zle_bind
{
	char	*seq;
	char	*raw;
	char	*widget;
}	t_zle_bind;

t_vec			*zle_binds(void);
void			zle_bind_raw(t_zle_bind *b);
void			zle_bind_add(const char *seq, const char *widget);
const char		*zle_bind_widget(const char *seq);
void			zle_binds_free(void);

/* The readline side. zle_active() is false outside the editor, which is
   what the bare `zle` guard in every plugin tests. */
t_shell			**zle_state_cell(void);
t_shell			**zle_caller_cell(void);
t_shell			*zle_caller(void);
bool			zle_active(void);
void			zle_install(t_shell *state);
int				zle_dispatch(int count, int key);
void			zle_do_redisplay(void);
void			zle_do_kill_buffer(void);
void			zle_do_accept_line(void);
int				zle_run_widget(t_shell *state, const char *name);

/* Carrying a widget's `cd` back over the readline fork (#80 item 2). The
   child reports its final directory on a pipe of its own, so the line
   protocol is untouched and no consumer of it needs to know this exists;
   the parent adopts the directory when it differs. See zle_cwd.c. */
int				*zle_cwd_pipe(void);
void			zle_cwd_open(void);
void			zle_cwd_send(void);
void			zle_cwd_adopt(t_shell *state);

#endif
