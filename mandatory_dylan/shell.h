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

/**
 * IF YOU'RE ASKing yourself wehre to go in this flow of information above
 * each library name is present a description of what it contains. It helps
 * separate the role of each one of them, for productivity dependency..
 * 
 * This way we keep things clean...
 * 
 * Practical Order of this program
 * 1. lex -> tokens
 * 2. parse -> AST (reparser buld word / token nodes)
 * 3. rn expander over AST (mutate token strings, expand to multiple ndoes
 * when needed; expand redir targets)
 * 4. finalize argv/redirections (globbing applied here)
 * 5. execute
 */

/** 
 * Contain all the tools we need from utils, constant defines, some debug
 * libraries from standardized to customized, data structures... 
 * All potential generic things that can be used throughout the buid
 * of the project
 */
# include "common.h"

/**
 * The builtings library contain all the signature of builtin function
 * that we have specially customized for our shell for critical performaces
 *  */
# include "builtins.h"

/**
 * Library API that help verify and execute
 */
# include "executor.h"

/**
 * 
 */
# include "history.h"

/**
 * 
 * 
*/
# include "lexer.h"

/**
 * 
 * 
 */
# include "parser.h"

/**
 * 
 */
# include "prompt.h"

/**
 * 
 */
# include "redir.h"

/**
 * 
 * 
 */
# include "sys.h"

/**
 * The expander is a component invoked after parsing/reparsing and before execution.
 * It performs the shell epxansion phases (tilde, parameter/variable, command substitution, arithmetic, field-splitting,
 * pathname/globbing) and mutates or replaces AST token/word nodes as needed
 * This is by far the most complex part, if we do the bonus of this project.
 * underDesigning the shell could compromise either the debugging or the working process
 * 
 * Expansion can change token text and token count (world-splitting, globbing -> multiple argv entries)
 *  and redirection targets must be expanded before we open files
 * 
 * Exception:
 * - Heredoc delimiters and raw heredeco behaviors are special-cased (handled during parsing/heredox handling)
 * - Command susbtitution may require spawning subprocesses during expansion
 * - Field-splitting and pathname expansion should run in the correct order (split then job)
 * 
 */
# include "expander.h"

# endif