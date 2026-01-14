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
 * 
 */
# include "alias.h"

/**
 * cldjlkasdfklj
 */
# include "public/ft_printf.h"

/**
 * 
 * ksldfksajkl
 */
# include "public/signals.h"
# include "public/error.h"
# include "ast.h"
# include "executor.h"
# include "redir.h"
# include "history.h"
# include "prompt.h"

# include <stdint.h>

/* option flags (bitmask) */
#define OPT_FLAG_HELP          (1u << 0)
#define OPT_FLAG_VERBOSE       (1u << 1)
#define OPT_FLAG_DEBUG_LEXER   (1u << 8)
#define OPT_FLAG_DEBUG_PARSER  (1u << 9)
#define OPT_FLAG_DEBUG_AST     (1u << 10)

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
	t_string	input;
	t_vec_env	env;
	t_string	cwd;
	t_ast_node	tree;
	//t_option		opt; 	// register the needs of the opt wanted, this way we can short-circuit in running time
	int			input_method;
	char		*base_context;
	char		*context;
	char		*pid;
	char		*last_cmd_status_s;
	t_exe_res	last_cmd_status_res;
	t_history	hist;
	bool		should_exit;
	t_vec_redir	redirects;
	int		heredoc_idx;
	t_buff_readline	readline_buff;
	t_prng_state	prng;
	/* bitwise option flags detected at startup */
	uint32_t	option_flags;
}   t_shell;

#endif
