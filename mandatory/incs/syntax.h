/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   syntax.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 17:47:04 by dlesieur          #+#    #+#             */
/*   Updated: 2025/10/29 19:46:59 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SYNTAX_H
# define SYNTAX_H

/**
 * The file's purpose is to create an extremly fast character classification
 * system for our shell lexer. 
 * instead of using slow if/else chains to check what a character is
 * the system pre-calculates the properties for all 256 possible characters
 * and stores them into a syntax table.
 * 
 * This file is part of `LUT` algorithm, it stores bit flags, the value at
 * each index is an intger ade by combining the bit flags from the
 * e_syntax flags
 * 
 * EXAMPLE:
 * A normal letter like 'a' might have the value CWORD (which is 0).
 * A space ' ' would have the value CSHBRK | CBLANK (which is 0x0002 | ... ).
 * A pipe '|' would have CSHMETA | CSHBRK (which is 0x0001 | 0x0002 = 0x0003).
 * 
 * Slow way: if (c == '(' || c == ')' || c == '<' || c == '>')
 * Fast way: if (shellmeta(c)) (which just checks a single bit:
 * 		sh_syntaxtab[c] & CSHMETA)
 */
/**
 * Defines for use the build tool to generate the syntax table.
 */
#define slashify_in_quotes        "\\`$\"\n"
#define slashify_in_here_document "\\`$"

#define shell_meta_chars          "()<>;&|"
#define shell_break_chars         "()<>;&| \t\n"
#define shell_quote_chars         "\"`'"

/**
 * Syntax,What it Does,Why it's Best Here
 * #if defined (FEATURE_MACRO),"Checks if FEATURE_MACRO is known
 * to the preprocessor (i.e., it exists).
 * Its value doesn't matter, just its existence.
 * ","This is exactly the logic needed for turning features on/off
 * (e.g., ""If PROCESS_SUBSTITUTION exists, enable its code."")."
 * #ifdef FEATURE_MACRO,"This is the shorter, more idiomatic version of the line above.
 * ","This is the preferred way to write a single, simple feature check."
 * #if FEATURE_MACRO,"Checks if FEATURE_MACRO has been defined
 * and if its defined value evaluates to non-zero (i.e., true).
 * ","This is less safe because if the macro isn't defined,
 * the compiler might interpret it as 0, which could hide
 * a bug where a feature is silently disabled."
 */
#if defined (PROCESS_SUBSTITUTION)
# define shell_exp_chars          "$<>"
#else
# define shell_exp_chars          "$"
#endif

#if defined (EXTENDED_GLOB)
# define ext_glob_chars           "@*+?!"
#else
# define ext_glob_chars           ""
#endif
#define shell_glob_chars          "*?[]^"
/**
 * Desfines shared by mksyntax.c and the rest of the shell code
 * theses bit flags define the properties of each character
 */
typedef enum e_syntax_flags
{
    CWORD       = 0x0000,   /* nothing special; an ordinary character */
    CSHMETA     = 0x0001,   /* shell meta character */
    CSHBRK      = 0x0002,   /* shell break character */
    CBACKQ      = 0x0004,   /* back quote */
    CQUOTE      = 0x0008,   /* shell quote character */
    CSPECL      = 0x0010,   /* special character that needs quoting */
    CEXP        = 0x0020,   /* shell expansion character */
    CBSDQUOTE   = 0x0040,   /* characters escaped by backslash in double quotes */
    CBSHDOC     = 0x0080,   /* characters escaped by backslash in here doc */
    CGLOB       = 0x0100,   /* globbing characters */
    CXGLOB      = 0x0200,   /* extended globbing characters */
    CXQUOTE     = 0x0400,   /* cquote + backslash */
    CSPECVAR    = 0x0800,   /* single-character shell variable name */
    CSUBSTOP    = 0x1000,   /* values of OP for ${word[:]OPstuff} */
    CBLANK      = 0x2000    /* whitespace (blank) character */
}t_syntax_flags;

static inline int	*access_shtab(void)
{
	int sh_syntaxtab[]={0};
	return (sh_syntaxtab);
}

/**
 * Character type accessors fucntions.
 * put static inline function to maintain performance and adhere to
 * coding standard
 */
static inline int	shell_meta(int c)
{
	int	*sh_syntaxtab = access_shtab();
	return (sh_syntaxtab[(unsigned char)(c)] & CSHMETA);
}

static inline int	shellbk(int c)
{
	int *sh_syntaxtab = access_shtab();
	return (sh_syntaxtab[(unsigned char)c] & CSHBRK);
}

static inline int shellxquote(int c)
{
	int	*sh_syntaxtab = access_shtab();
    return (sh_syntaxtab[(unsigned char)(c)] & CXQUOTE);
}

static inline int shellblank(int c)
{
	int	*sh_syntaxtab = access_shtab();
    return (sh_syntaxtab[(unsigned char)(c)] & CBLANK);
}

static inline int parserblank(int c)
{
    return ((c) == ' ' || (c) == '\t');
}

static inline int issyntype(int c, int t)
{
	int	*sh_syntaxtab = access_shtab();
    return ((sh_syntaxtab[(unsigned char)(c)] & (t)) != 0);
}

static inline int notsyntype(int c, int t)
{
	int	*sh_syntaxtab = access_shtab();
    return ((sh_syntaxtab[(unsigned char)(c)] & (t)) == 0);
}

# if defined (PROCESS_SUBSTITUION)
static inline int shellexp(int c)
{
	return ((c) == '?' || (c) == '<' || (c) == '>');
}
# else
static inline int shellexp(int c)
{
    return ((c) == '$');
}
# endif

# if defined (EXTENDED_GLOB)
static inline int pattern_char(int c)
{
    return ((c) == '@' || (c) == '*' || (c) == '+' || (c) == '?' || (c) == '!');
}
# else
static inline int pattern_char(int c)
{
    /* To avoid unused parameter warnings, though 0 is the logical return */
    (void)c;
    return 0;
}
# endif

static inline int glob_char(int c)
{
    return ((c) == '*' || (c) == '?' || (c) == '[' || (c) == ']' || (c) == '^');
}

typedef enum e_control_chars
{
	CTLESC = '\001',
	CTLNUL = '\177'
}t_control_chars;

# if !defined (HAVE_ISBLANK) && !defined (isblank)
static inline int	isblank(int x)
{
	return (x == ' ' || x == '\t');
}
# endif

#endif
