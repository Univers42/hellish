/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 00:27:35 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 02:33:21 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GLOB_H
# define GLOB_H

#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>

enum
{
	GLOB_ERR,
	GLOB_MARK,
	GLOB_NOSORT,
	GLOB_DOOFS,
	GLOB_NOCHECK,
	GLOB_APPEND,
	GLOB_NOESCAPE,
	GLOB_PERIOD,
	GLOB_MAGCHAR,
	GLOB_ALTDIRFN,
	GLOB_BRACE,
	GLOB_NOMAGIC,
	GLOB_TILDE,
	GLOB_ONLYDIR,
	GLOB_TILDE_CHECK,
	GLOB_NOSPACE,
	GLOB_ABORTED,
	GLOB_NOMATCH,
	GLOB_NOSYS
}

typedef enum e_str_match_flag
{
	STRMATCH_NO = -1,
	STRMATCH_PNAME = 1 << 0,		// No wildcard can ever match
	STRMATCH_NOESCAPE = 1 << 1,	// Backslashes don't quote special chars
	STRMATCH_PERIOD= 1 << 2,	// Leading `.` is matched only explicitly
	STRMATCH_LEADING_DIR = 1 << 3,	// ignore `/...` after a match
	STRMATCH_CASEFOLD = 1 << 4,		// compare without regard to case
	STRMATCH_EXTMATCH = 1 << 5,		// use ksh-like extended matching
	STRMATCH_FIRST_CHAR = 1 << 6,	// match only the first character
	STRMATCH_DOTDOT = 1 << 7		// Force `.` and `..` to match explicitly
}	t_match_flag;

typedef enum e_char_class
{
	CC_NO_CLASS,
	CC_ASCII,
	CC_ALNUM,
	CC_ALPHA,
	CC_BLANK,
	CC_CNTRL,
	CC_DIGIT,
	CC_GRAPH,
	CC_LOWER,
	CC_PRINT,
	CC_PUNCT,
	CC_SPACE,
	CC_UPPER,
	CC_WORD,
	CC_XDIGIT
}	t_char_class;

typedef struct s_collsym
{
	char			*name;
	unsigned char	code;
}	t_collsym;

static inline char const *const	get_cclass_name()
{
	static char const *const cclass_name[] =
	{
    	"",
    	"ascii", "alnum", "alpha", "blank", "cntrl", "digit", "graph",
    	"lower", "print", "punct", "space", "upper", "word", "xdigit"
  	};
}

size_t	n_char_class(char const const *cclass_name)
{
	return (sizeof(cclass_name) / sizeof(cclass_name[0]));
}


# define GLOB_ASCIIRANGE 256

typedef struct dirent64 t_dirent64;

typedef struct s_glob
{
	size_t	pathc;
	char	**pathv;
	size_t	offs;
	int		flags;
}   t_glob;

typedef struct s_gglobe
{
	int	glob_recursion_depth;
}	t_gglobe;


#endif