/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   shell.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:34:08 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:34:08 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MINISHELL_H
# define MINISHELL_H

/**
 * We would need to verify the circular dependencies on each file to 
 * avoid errors when compiling
 */

// public api
# include "libft.h"
# include "sys.h"
# include "common.h"
# include "config.h"
# include "infrastructure.h"

// Concept of shell gather here
# include "../src/helpers.h"
# include "../src/prompt/prompt.h"
# include "../src/lexer/lexer.h"
# include "infrastructure.h"
# include "../src/parser/parser.h"
# include "../src/reparser/reparser.h"

typedef struct s_state
{
	t_string		input;
	t_vec_env		env;
	t_string		cwd;
	t_ast_node		tree;
	int				input_method;
	char			*base_context;
	char			*context;
	char			*pid;
	char			*last_cmd_status_s;
	t_exe_res		last_cmd_status_res;
	t_history		hist;
	bool			should_exit;
	t_vec_redir		redirects;
	int				heredoc_idx;
	t_buff_readline	readline_buff;
	t_prng_state	prng;
}	t_state;


void		init_setup(t_state *state, char **argv, char **envp);

#endif
