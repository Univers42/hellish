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
# define OLDPWD "OLDPWD"
# define USER "USER"
# define LPAREN '('
# define RPAREN ')'

# define CUR_DIR "~+"
# define PREV_DIR "~-"
# define START_PREV_DIR "~-/"
# define START_CUR_DIR "~+/"
# define HOME_DIR "~"
# define START_HOME_DIR "~/"
# define BEFORE "~-"
# define IFS "IFS"
# define TMP_DIR "/tmp"
# define SHLVL "SHLVL"
# define DFT_PATH "/usr/local/sbin:/usr/local/bin:\
/usr/sbin:/usr/bin:/sbin:/bin"
# define PATH "PATH"
# define ULTIMATE_ARG "_"
# define MINISHELL "./minishell"
# define EQ '='
# define FB_SH "/bin/sh"
//# define PATH_HELLISH "/usr/bin/hellish"
# define PATH_HELLISH "/home/dlesieur/Documents/sh42/build/bin/minishell"
# define PROC_SELF_EXE "/proc/self/exe"
# define BLACK_HOLE "/dev/null"
# define CMD_OPT "-c"
# define EXIT_CMD_NOT_EXEC 126
# define PERMISSION_DENIED 126
# define NO_SUCH_FILE_OR_DIR 127
# define NO_SUCH_DIR 127
# define OUT_OF_MEM 127
# define IS_A_DIR 126
# define EXIT_CMD_NOT_FOUND 127
# define EXIT_GENERAL_ERR 1
# define IS_DOLLAR '$'

// Classes
# define CLASSE_ALNUM "[:alnum:]"
# define PAT_ALNUM "0123456789ABCDEFGHIJKLMNOPQRSTUV\
						WXYZabcdefghijklmnopqrstuvwxyz"
# define CLASSE_ALPHA "[:alpha:]"
# define PAT_ALPHA "ABCDEFGHIJKLMNOPQRSTUVWXYZZabcdefghijklmnopqrstuvwxyz"
# define CLASSE_DIGIT "[:digit:]"
# define PAT_DIGIT "0123456789"
# define CLASSE_LOWER "[:lower:]"
# define PAT_LOWER "abcdefghijklmnopqrstuvwxyz"
# define CLASSE_UPPER "[:upper:]"
# define PAT_UPPER "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
# define CLASSE_SPACE "[:space:]"
# define PAT_SPACE " \t\n\r\f\v"
# define CLASSE_BLANK "[:blank:]"
# define PAT_BLANK " \t"
# define CLASSE_PUNCT "[:punct:]"
# define PAT_PUNCT "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
# define CLASSE_XDIGIT "[:xdigit:]"
# define PAT_XDIGIT "0123456789ABCDEFabcdef"
# define CLASSE_CNTRL "[:cntrl:]"
# define PAT_CNTRL "\001\002\003\004\005\006\007\010\
						\011\012\013\014\015\
						\016\017\020\021\022\023\024\025\026\027\030\
						\031\032\033\034\035\036\037\177"
# define CLASSE_GRAPH "[:graph:]"
# define PAT_GRAPH "!\"#$%&'()*+,-./0123456789:;<=>?@ \
						ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^ \
						_`abcdefghijklmnopqrstuvwxyz{|}~"
# define CLASSE_PRINT "[:print:]"
# define PAT_PRINT " !\"#$%&'()*+,-./0123456789:;\
						<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]\
						^_`abcdefghijklmnopqrstuvwxyz{|}~"
# define TMP_HC_DIR "/tmp/heredoc_"
# define STRIP_HEREDOC "<<-"
# define PIPE_HEREDOC_PROMPT "pipe_hereodoc> "
# define HEREDOC_PROMPT "heredoc> "
# define SPECIAL_CHARS ";$'\"<>|&()\n"
# define TOKEN_DQ "TT_DQWORD"
# define TOKEN_SQ "TT_SQWORD"
# define EMPTY "(empty)"
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