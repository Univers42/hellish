/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 01:39:59 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 01:39:59 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_H
# define CONFIG_H

/**
 * The configurations are all important and should not impact the good
 * usability of the project..
 * Well this is part true because if you for isntance the unicode > 
 * for another you might get a `drift cursor`for not telling explicitly
 * the terminal how much columns take the wide byte you've designed to be 
 * the prompt...
 * I'm just saying...
 */

# define COMMAND_NOT_FOUND 127
# define EXE_PERM_DENIED 126
# define EXE_BASE_ERROR 125
# define CANCELED 130
# define SYNTAX_ERR 2
# define AMBIGUOUS_REDIRECT 1

# ifndef PROMPT
#  define PROMPT "\001❯ \002"
# endif

# define HIST_FILE ".minishell_history"

# define ANSI_RED "\001\033[31m\002"
# define ANSI_GREEN "\001\033[32m\002"
# define ANSI_RESET "\001\033[0m\002"

# ifndef RL_SPACER_1
#  define RL_SPACER_1 "\x03"
# endif

# define LEXER_SQUOTE_PROMPT "squote> "
# define LEXER_DQUOTE_PROMPT "dquote> "
// to implement 
# define LEXER_BQUOTE_PROMPT "bquote> "

# endif