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

# include <stdbool.h>  // <-- added: ensure 'bool' is defined for subsequent typedefs

typedef struct s_shell t_shell;

/* expose redirection struct project-wide so sizeof(t_redir) is available */
# ifndef REDIR_TYPE_DEFINED
#  define REDIR_TYPE_DEFINED
typedef struct redir_s
{
	bool		direction_in;
	int			fd;
	char		*fname;
	bool		should_delete;
}	t_redir;
# endif

/* make input enum available project-wide to avoid compile-order issues */
#ifndef INPUT_ENUM_DEFINED
# define INPUT_ENUM_DEFINED
typedef enum e_input_method
{
	INP_READLINE,
	INP_FILE,
	INP_ARG,
	INP_STDIN_NOTTY,
}	t_input_method;
#endif

/**
 * We would need to verify the circular dependencies on each file to 
 * avoid errors when compiling
 */

// public api
# include "libft.h"
# include "alias.h"
# include "sys.h"
# include "config.h"
# include "infrastructure.h"

// Concept of shell gather here
# include "../src/helpers/helpers.h"
# include "../src/prompt/prompt.h"
# include "../src/lexer/lexer.h"
# include "infrastructure.h"
# include "../src/parser/parser.h"
# include "../src/reparser/reparser.h"
# include "../src/input/input.h"
# include "../src/expander/expander.h"

// make parse_and_execute_input visible project-wide (avoid implicit decl)
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


void		init_setup(t_shell *state, char **argv, char **envp);

#endif
