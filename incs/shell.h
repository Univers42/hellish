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

#ifndef SHELL_H
# define SHELL_H

# include "alias.h"
# include "ft_printf.h"
# include "public/signals.h"
# include "public/error.h"
# include "ast.h"
# include "executor.h"
# include "redir.h"
# include "history.h"
# include "prompt.h"

# include <stdint.h>

enum e_opt_flag
{
	OPT_FLAG_HELP = 1u << 0,
	OPT_FLAG_VERBOSE = 1u << 1,
	OPT_FLAG_DEBUG_LEXER = 1u << 8,
	OPT_FLAG_DEBUG_PARSER = 1u << 9,
	OPT_FLAG_DEBUG_AST = 1u << 10
};

void	parse_and_execute_input(t_shell *state);

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

typedef struct s_procsub_entry
{
	pid_t	pid;
	int		fd;
	char	*path;
}	t_procsub_entry;

typedef t_vec	t_vec_procsub;

typedef struct s_shell
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
	uint32_t		option_flags;
	int				bg_job_count;
	t_vec_procsub	proc_subs;
}	t_shell;

/* Directory matcher context for glob expansion */
typedef struct s_dir_matcher
{
	char		*path;
	DIR			*dir;
	t_vec_glob	glob;
	size_t		offset;
	t_vec		*args;
}	t_dir_matcher;

static inline t_shell	shell_init(void)
{
	return ((t_shell){0});
}

#endif
