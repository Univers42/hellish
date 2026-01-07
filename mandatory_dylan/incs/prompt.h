/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 22:15:26 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 22:15:26 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PROMPT_H
# define PROMPT_H

# include "common.h"
# include <sys/types.h>
# include <sys/wait.h>
# include <signal.h>
# include "readline/readline.h"

# define ANSI_RED "\001\033[31m\002"
# define ANSI_GREEN "\001\033[32m\002"
# define ANSI_RESET "\001\033[0m\002"

# define RL_SPACER_1 "\x03"

# define SQUOTE_PROMPT "squote>"
# define DQUOTE_PROMPT "dquote> "
# define BQUOTE_PROMPT "bquote> "

typedef struct s_rl
{
	bool	has_line;
	bool	should_update_context;
	bool	has_finished;
	int		line;
	t_string	buff;
	size_t	cursor;
}	t_rl;

void bg_readline(int outfd, char *prompt);
int attach_input_readline(t_rl *l, int pp[2], int pid);
int get_more_input_readline(t_rl *l, char *prompt);
void buff_readline_update(t_rl *l);
void buff_readline_reset(t_rl *l);
void buff_readline_init(t_rl *ret);
void update_context(t_rl *rl, char **context, char **base_context);
int get_more_input_notty(t_rl *rl);


#endif