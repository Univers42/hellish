/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sh_state.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/11 20:17:31 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/11 20:18:16 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SH_STATE_H
# define SH_STATE_H

# include "common.h"

typedef enum e_option
{
	OPT_DEBUG,
	OPT_DEBUGGER,
	OPT_HELP,
	OPT_INIT_FILE,
	OPT_LOGIN,
	OPT_POSIX,
	OPT_PRETTY_PRINT,
	OPT_RCFILE,
	OPT_RESTRICTED,
	OPT_VERBOSE,
	OPT_COMMAND
}	t_option;

typedef struct s_shell
{
	t_string		input;
	t_vec_env		env;
	t_string		cwd;
	t_ast_node		tree;
	//t_option		opt;		register the needs of the opt wanted, this way we can short-circuit in running time
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
}	t_shell;

void		on(t_shell *state, char **argv, char **envp);
#endif