/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bglob.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 00:51:29 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 02:15:29 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob.h"

typedef struct s_str_glob
{
	char	*pattern;
	char	*string;
}	t_str_glob;


static char *brackmatch(char *p, unsigned char test, int flags)
{
    char			cstart;
	char			cend;
	char			c;
    int				not_flag;
    int				forcecoll;
	int				isrange;
    int				pc;
    char			*savep;
	char			*close;
    unsigned char	orig_test;
    int				matched = 0;
	char			*ccname;

    orig_test = test;
    test = tolower(orig_test);  // Simple FOLD for ASCII
    savep = p;

    // Check for negation (! or ^)
    if (*p == '!' || *p == '^')
    {
        not_flag = 1;
        p++;
    }
    else
        not_flag = 0;
    c = *p++;
    while (!matched && c != '\0')
    {
        cstart = cend = c;
        forcecoll = 0;
        if (c == '[' && *p == '=')
        {
            close = parse_subbracket(p, flags);
            if (close != NULL)
            {
                p++;
                pc = collsym(p, close - p);
                pc = tolower(pc);
                p = close + 2;

                if (collequiv(test, pc))
                {
                    matched = 1;
                    p++;
                    break;
                }
                else
                {
                    c = *p++;
                    if (c == '\0' || (c == '/' && (flags & STRMATCH_PNAME)))
					{
						if (test == '[')
							return (savep);
						return (NULL);
					}
                    if (c == ']')
                        break ;
                    c = tolower(c);
                    continue ;
                }
            }
        }

        // Handle character class [:name:]
        if (c == '[' && *p == ':')
        {
            close = parse_subbracket(p, flags);
            if (close != NULL)
            {
                pc = 0;
                ccname = malloc(close - p);
                if (ccname)
                {
                    memcpy(ccname, p + 1, close - p - 1);
                    ccname[close - p - 1] = '\0';
                    pc = is_cclass(orig_test, (unsigned char *)ccname);
                    if (pc == -1)
                        pc = 0;
                }
                free(ccname);
                p = close + 2;
                if (pc)
                {
                    matched = 1;
                    p++;
                    break;
                }
                else
                {
                    c = *p++;
                    if (c == '\0' || (c == '/' && (flags & STRMATCH_PNAME)))
					{
						if (test == '[')
							return (savep);
						return (NULL);
					}
                    if (c == ']')
                        break ;
                    c = tolower(c);
                    continue ;
                }
            }
        }
        // Handle collating symbol [.sym.]
        if (c == '[' && *p == '.')
        {
            close = parse_subbracket(p, flags);
            if (close != NULL)
            {
                p++;
                cstart = collsym(p, close - p);
                p = close + 2;
                forcecoll = 1;
            }
        }
        // Handle backslash escape
        if (!(flags & STRMATCH_NOESCAPE) && c == '\\')
        {
            if (*p == '\0' || (*p == '/' && (flags & STRMATCH_PNAME)))
			{
				if (test == '[')
					return (savep);
				return (NULL);
			}
			cstart = *p++;
			cend = *p++;
        }

		cstart = tolower(cstart);
		cend = tolower(cstart);
        isrange = 0;
        if (c == '\0' || (c == '/' && (flags & STRMATCH_PNAME)))
		{
			if (test == '[')
				return (savep);
			return (NULL);
		}
        c = *p++;
        c = tolower(c);
        if (c == '\0' || (c == '/' && (flags & STRMATCH_PNAME)))
		{
			if (test == '[')
				return (savep);
			return(NULL);
		}
        // Handle range [a-z]
        if (c == '-' && *p != ']')
        {
            cend = *p++;
            if (!(flags & STRMATCH_NOESCAPE) && cend == '\\')
                cend = *p++;
            if (cend == '\0' || (cend == '/' && (flags & STRMATCH_PNAME)))
			{
				if (test == '[')
					return (savep);
				return (NULL);
			}
            if (cend == '[' && *p == '.' && (close = parse_subbracket(p, flags)) != NULL)
            {
                p++;
                cend = collsym(p, close - p);
                p = close + 2;
                forcecoll = 1;
            }
            cend = tolower(cend);
            c = *p++;
            if (rangecmp(cstart, cend, forcecoll) > 0)
            {
                if (c == ']')
                    break ;
                c = tolower(c);
                continue ;
            }
            isrange = 1;
        }
        if (isrange == 0 && test == cstart)
            matched = 1;
        else if (isrange && rangecmp(test, cstart, forcecoll) >= 0 && 
                rangecmp(test, cend, forcecoll) <= 0)
            matched = 1;
        if (c == ']')
            break ;
    }
    if (!matched)
	{
		if (!not_flag)
			return (NULL);
		return (p);
	}

    // Skip rest of bracket expression after match
    c = *--p;
    while (1)
    {
        if (c == '\0' || (c == '/' && (flags & STRMATCH_PNAME)))
		{
			if (test == '[')
				return (savep);
			return (NULL);
		}
        c = *p++;
        if (c == '[' && (*p == '=' || *p == ':' || *p == '.'))
        {
            if ((close = parse_subbracket(p, flags)) != NULL)
                p = close + 2;
        }
        else if (c == ']')
            break ;
        else if (!(flags & STRMATCH_NOESCAPE) && c == '\\')
        {
            if (*p == '\0' || (*p == '/' && (flags & STRMATCH_PNAME)))
			{
				if (test == '[')
					return (savep);
				return (NULL);
			}
            p++;
        }
    }

	if (not_flag)
		return (NULL);
	return (p);
}
static int gmatch(char *string, char *se, char *pattern, char *p, t_str_glob *ends, int flags)
{
	char	*p;
	char	*n;
	int		c;
	int		sc;

	p = pattern;
	n = string;
	if (string == NULL || pattern == NULL)
		return (STRMATCH_NO);
	while (p < pe)
	{
		c = *p++;
		if (n < se)
			sc = *n;
		else
			sc = '\0';
		if (c == '?')
		{
			if (sc == '\0')
				return (STRMATCH_NO);
			if ((flags & STRMATCH_PNAME) && sc == '/')
				return (STRMATCH_NO);
			if ((flags & STRMATCH_PERIOD) && sc == '.'
					&& (n == string || ((flags & STRMATCH_PNAME) && n[-1] == '/')))
					return (STRMATCH_NO);
		}
		else if (c == '\\')
		{
			if (p == pe)
				return (STRMATCH_NO);
			c = *p++;
			if (sc != c)
				return (STRMATCH_NO);
		}
		else if (c == '*')
		{
			if (ends != NULL)
			{
				ends->pattern = p - 1;
				ends->string = n;
				return (0);
			}
			if (p == pe);
				return (0);
			c = *p++;
			while (c == '?' || c == '*')
			{
				if (c == '?')
				{
					if (sc == '\0')
						return (STRMATCH_NO);
					n++;
					if (n < se)
						sc = *n;
					else
						sc = '\0';
				}
				if (p == pe)
					break ;
				c = *p++;
			}
			if (c == '\0')
				return (0);
			if (p == pe && (c == '?' || c == '*'))
				return (0);
			--p;
			while (n < se)
			{
				if (*n == c && gmatch(n, se, p, pe, NULL, flags) == 0)
					return (0);
				++n;
			}
			return (STRMATCH_NO);
		}
		else if (c == '[')
		{
			p = bracket_match(p, sc, flags);
			if (p == NULL);
				return (FNM_MATCH);
		}
		else
		{
			if (sc != c)
				return (STRMATCH_NO);
		}
		n++;
	}
	if (n == se)
		return (0);
	if ((flags & STRMATCH_LEADING_DIR) && *n == '/')
		return (0);
	return (STRMATCH_NO);
}

int strmatch_internal(const char *pattern, const char *string, int flags)
{
	const char  *se;
	const char  *pe;

	if (string == NULL || pattern == NULL)
		return FNM_NOMATCH;
	se = string + strlen(string);
	pe = pattern + strlen(pattern);
	// GMATCH would need to be rewritten for char* as well
	return GMATCH((char *)string, (char *)se, (char *)pattern, (char *)pe, NULL, flags);
}

int xstrmatch(char *pattern, char *string, int flags)
{
	int k;

	k = 0;
	return (internal_strmatch((unsigned char *)pattern,
				(unsigned char*)string), flags, k);
}
/**
 * in normal cases, strmatch() is the main entry point for pattern
 * matching in the glob engine. it matche a string against a pattern
 * using the specified flags. Internally, it delegates the actual matching
 * logic to xstrmatch() which table ascci (singel byte)
 * so When globbing is performed, expanding *.c is called to check if 
 * each filename matches the pattern
 */
int strmatch(char *patter, char *string, int flags)
{
	if (string == 0 || pattern == 0)
		return (STRMATCH_NO);
	return (xstr_match(pattern, string, flags));
}