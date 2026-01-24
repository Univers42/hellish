/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sys.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 01:37:14 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 01:37:14 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SYS_H
# define SYS_H
# define MSG_GETCWD_SHINIT "shell-init: error retrieving current directory:\
						getcwd: cannot access parent directories:\
						No such file or directory\nsh: 0: \
						getcwd() failed: No such file or directory\n"
# define HOME "HOME"
# define PWD "PWD"
# define IFS "IFS"
# define TMP_DIR "/tmp"
# define SHLVL "SHLVL"
# define DFT_PATH "/usr/local/sbin:/usr/local/bin\
					:/usr/sbin:/usr/bin:/sbin:/bin"
# define PATH "PATH"
# define ULTIMATE_ARG "_"
# define MINISHELL "./minishell"
# define EQ '='
# define FB_SH "/bin/sh"

# define EXIT_CMD_NOT_EXEC 126
# define PERMISSION_DENIED 126
# define NO_SUCH_FILE_OR_DIR 127
# define NO_SUCH_DIR 127
# define OUT_OF_MEM 127
# define IS_A_DIR 126
# define EXIT_CMD_NOT_FOUND 127
# define EXIT_GENERAL_ERR 1

// debug printing of the tree structure
# ifndef PRINT_AST
#  define PRINT_AST 0
# endif

// Debug printing of the token queue
# ifndef PRINT_TOKENS
#  define PRINT_TOKENS 1
# endif

# ifndef TRACE_DEBUG
#  define TRACE_DEBUG 0
# endif

#endif