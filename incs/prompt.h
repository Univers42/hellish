/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:29:55 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:29:55 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PROMPT_H
# define PROMPT_H

# include "alias.h"
# include <stdbool.h>
# include <stddef.h>

/* forward-declare shell to avoid pulling full shell.h here */
typedef struct s_shell	t_shell;

// buff_readline.c
typedef struct s_buff_readline
{
	bool		has_line;
	bool		should_update_context;
	bool		has_finished;
	int			line;
	t_string	buff;
	size_t		cursor;
}	t_buff_readline;

// Forward declaration to avoid circular dependency
struct					s_parser;

int			buff_readline(t_shell *state, t_string *ret, char *prompt);
void		buff_readline_update(t_buff_readline *l);
void		buff_readline_reset(t_buff_readline *l);
int			get_more_input_readline(t_buff_readline *l, char *prompt);
void		update_context(t_shell *state);
int			get_more_input_notty(t_shell *state);

void		bg_readline(int outfd, char *prompt);
int			attach_input_readline(t_buff_readline *l, int pp[2], int pid);
t_string	prompt_normal(void);
t_string	prompt_more_input(struct s_parser *parser);
void		buff_readline_init(t_buff_readline *ret);
void		update_context(t_shell *state);

#endif