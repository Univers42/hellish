/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   progcomp_private.h                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 10:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 10:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Programmable completion -- #72 phase 4, the half that reaches readline.
   Its own header rather than completion_private.h's because t_compspec is
   wider than that file's declaration column, and the 42 norm aligns a whole
   block to one of them. Included only by src/completion/progcomp*.c. */

#ifndef PROGCOMP_PRIVATE_H
# define PROGCOMP_PRIVATE_H

# include "libft.h"
# include "shell.h"

/* The matches built for ONE TAB press: a t_vec of xmalloc'd char *, refilled
   from scratch each time and emptied again as soon as readline has copied
   what it wants. It is a function-local static, not a global -- the readline
   generator protocol takes no context and gives back one match per call. */
t_vec		*pc_cell(void);
void		pc_reset(void);
void		pc_push(const char *s, int n);

/* What the line under the cursor says. `start` is readline's offset of the
   word being completed. */
char		*pc_cmd_word(int start);
char		*pc_prev_word(int start);

/* The shell command that produces the answer, and the run of it. It is
   built as TEXT and handed to exec_string rather than assembled through the
   env and executor APIs: -W's list, COMP_WORDS and a -F call all have to
   mean what the shell says they mean, and the one way to guarantee that is
   to let the shell parse them. Every value crossing that boundary goes
   through pc_qpush, which single-quotes it. */
void		pc_qpush(t_string *out, const char *s, int n);
void		pc_head(t_string *out, int start);
char		*pc_call_str(t_compspec *c, const char *text, int start);
bool		pc_build(t_shell *st, t_compspec *c, const char *text, int start);
t_compspec	*comp_find(t_shell *st, const char *name);
char		**progcomp_try(const char *text, int start, int end);

#endif
