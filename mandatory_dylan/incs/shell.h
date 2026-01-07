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

#endif /* SHELL_H */