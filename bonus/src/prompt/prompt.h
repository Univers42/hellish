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

// get next line (without \n).
// Appends the line to ret
//
// return value:
// 0 - on no input (ctrl-d)
//
// 1 - on empty line
//
// 2 - ctrl-c
//
// 3 - anything else
int			buff_readline(t_state *state, t_string *ret, char *prompt);
void		buff_readline_update(t_buff_readline *l);
void		buff_readline_reset(t_buff_readline *l);
int			get_more_input_readline(t_buff_readline *l, char *prompt);
void		update_context(t_state *state);
int			get_more_input_notty(t_state *state);

void	bg_readline(int outfd, char *prompt);
int	attach_input_readline(t_buff_readline *l, int pp[2], int pid);
int	get_more_input_readline(t_buff_readline *l, char *prompt);
t_string	prompt_normal(t_state *state);
t_string	prompt_more_input(t_parser *parser);
# endif