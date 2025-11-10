/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.c                                          :+:      :+:    :+:   */
/*                                                                            */
/* ************************************************************************** */

#include "termcap.h"

char *find_capability(char *bp, char *cap)
{
	if (!bp || !cap)
		return (NULL);
	while (*bp)
	{
		if (bp[0] == ':' && bp[1] == cap[0] && bp[2] == cap[1])
			return (&bp[4]);
		bp++;
	}
	return (NULL);
}

static int compare_contin(char *s1, char *s2)
{
	int c1;
	int c2;

	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;
		while (c1 == '\\' && *s1 == '\n')
		{
			s1++;
			while ((c1 = *s1++) == ' ' || c1 == '\t')
				;
		}
		if (c2 == '\0')
			return ((c1 == '|' || c1 == ':') ? 0 : 1);
		if (c1 != c2)
			return (1);
	}
	return (0);
}

static int name_match(char *line, char *name)
{
	char *tmp;

	if (!compare_contin(line, name))
		return (1);
	tmp = line;
	while (*tmp && *tmp != '\n' && *tmp != ':')
	{
		if (*tmp == '|' && !compare_contin(tmp + 1, name))
			return (1);
		tmp++;
	}
	return (0);
}

static int process_line(char *line, char *str)
{
	if (*line != '#' && name_match(line, str))
		return (1);
	return (0);
}

int scan_file(char *str, int fd, t_buffer *bufp)
{
	char *line;

	if (!str || fd < 0 || !bufp)
		return (0);
	bufp->ptr = bufp->beg;
	bufp->full = 0;
	bufp->ateof = 0;
	*bufp->ptr = '\0';
	line = gobble_line(fd, bufp, NULL);
	while (line && *bufp->ptr)
	{
		if (process_line(bufp->ptr, str))
			return (1);
		bufp->ptr = line;
		line = gobble_line(fd, bufp, line);
	}
	return (0);
}