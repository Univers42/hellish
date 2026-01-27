/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sh_state.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/11 20:17:31 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:17:29 by dlesieur         ###   ########.fr       */
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
	t_string			input;
	t_vec_env			env;
	t_string			cwd;
	t_ast_node			tree;
	int					metinp;
	char				*dft_ctx;
	char				*ctx;
	char				*pid;
	char				*last_cmd_st;
	t_execution_state	last_cmd_st_exe;
	t_history			hist;
	bool				should_exit;
	t_vec_redir			redirects;
	int					heredoc_idx;
	t_rl				rl;
	t_prng				prng;
	uint32_t			option_flags;
}	t_shell;

void		on(t_shell *state, char **argv, char **envp);

#endif