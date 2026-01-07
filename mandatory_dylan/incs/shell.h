/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   api.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 22:27:14 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 22:27:14 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SHELL_H
# define SHELL_H

/*
 * shell.h
 *
 * High-level "umbrella" header that documents the major modules of the minishell
 * project and exposes their public headers in a single place for convenience.
 *
 * Intended use:
 *  - Include common.h first (core types, helpers, small data-structures).
 *  - Then include the feature headers in a practical order:
 *      lexer -> parser -> expander -> executor (runtime)
 *    plus support modules (builtins, history, prompt, redirections, etc.).
 *
 * This file contains only organized includes and short comments describing each
 * module's responsibility. Implementation and API details remain in the
 * respective headers/source files.
 */

/* Core helpers, macros and fundamental types used everywhere. */
# include "common.h"

/* Data-structures and small utilities (vectors, deques, hash, AST helpers). */
# include "dsa.h"

/* Signal handling helpers and simple wrapper functions. */
# include "sigs.h"

/* Formatted printing utilities used for logging / error messages. */
# include "ft_printf/ft_printf.h"

/* Builtin commands (echo, cd, export, env, unset, pwd, exit).
   Provides signatures to register/lookup builtin functions. */
# include "builtins.h"

/* Lexer: tokenizes input strings into token streams. */
# include "lexer.h"

/* Parser: builds an AST from token streams. */
# include "parser.h"

/* Expander: performs shell expansions (tilde, variables, command substitution,
   field splitting and pathname expansion) on AST nodes. */
# include "expander.h"

/* Redirection helpers and data types describing I/O redirections. */
# include "redir.h"

/* Execution layer: types and helpers to run commands and pipelines. */
# include "executor.h"

/* Prompt and input handling (readline integration, non-tty input). */
# include "prompt.h"
# include "input.h"

/* History management (load/save history, integrate with readline). */
# include "history.h"

/* Environment helpers (env list, lookup, set/unset helpers). */
# include "env.h"

/* System-level project flags (debug/bonus feature toggles). */
# include "sys.h"

/* Error/exit helpers (uniform error reporting and abort helpers). */
# include "exit.h"
typedef struct s_core
{
	t_string		input;
	t_string		cwd;
	t_envs			env;
	t_hist			hist;
	t_input_method	input_method;
	char			*base_context;
	char			*context;
	char			*pid;
	char			*last_cmd_status_s;
	t_state			last_exit_code;
	bool			should_exit;
	t_rl			rl;
	t_sigaction		*saved_sigaction;
}	t_core;



typedef struct s_shell
{
	bool			(*init)();
	bool			(*process)();
	bool			(*shutdown)();
	t_core			core;
	int				argc;
	char			**argv;
	char			**envp;
	t_prng_state	prng;
}	t_shell;

static inline void	buffering(void)
{
	t_core	data;

	set_unwind_sig();
	data.pid = getpid_hack();
	data.context = strdup(argv[0]);
	data.base_context = strdup(argv[0]);
	set_cmd_status();
	data.last_exit_code = exit_status(0);
	init_cwd(&data.cwd);
	data.env = env_to_vec_env(&data, envp);
	if (argv[1] && strcmp(argv[1], "-c") == 0)
		init_arg(&data, argv);
	else if (argv[1])
		init_file(&data, argv);
	else if (!isatty(0))
		init_stdin_notty(&dta);
	else
		init_hitory(&data);
	prng_init_state(shell_state()->prng, 19650218UL);
}


static inline buffering(t_api *data, t_shell)
{
	shell.core.
}

static inline repl(void)
{
	t_shell	*
	while ()
	{
		
	}
}

static inline void	free_all_and_exit(t_api *data)
{
	free_env(&data->env);
	free_all_state(data);
	forward_exit_status(data->last_exit_code);
}

/**
 * singleton pattern, not thread safe but we don't care because
 * it's the global state of shell there is only one program not two..
 * and if needed we cans still go back if wanted to let's say..
 * To do that.. we'll let the opportunity of the user to set initially the
 * shell state or to chose the default settings
 * Then it gets either retrieve those information from the local data
 * coverage
 * or the heap
 */
static inline shell_state(t_shell *set)
{
	static t_shell	shell;

	if (set)
		return (set);
	shell.init = buffering();
	shell.process = repl();
	shell.shutdown = free_all_and_exit();
}
#endif /* SHELL_H */