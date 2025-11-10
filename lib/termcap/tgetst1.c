/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tgetst1.c                                          :+:      :+:    :+:   */
/*                                                                            */
/* ************************************************************************** */

#include "termcap.h"

static int handle_backslash_escape(char **p)
{
	int c;
	int size;
	int c1;
	t_tglob *g;

	c = **p;
	(*p)++;
	if (c >= '0' && c <= '7')
	{
		c -= '0';
		size = 0;
		while (++size < 3 && (c1 = **p) >= '0' && c1 <= '7')
		{
			c = c * 8 + (c1 - '0');
			(*p)++;
		}
		return (c);
	}
	if (c >= 0100 && c < 0200)
	{
		g = access_glob();
		c1 = g->esctab[(c & ~040) - 0100];
		if (c1 != ' ')
			return (c1);
	}
	return (c);
}

static int process_escape_char(char **p)
{
	int c;

	c = **p;
	(*p)++;
	if (c == '^')
	{
		c = **p;
		(*p)++;
		if (c == '?')
			return (0177);
		return (c & 037);
	}
	if (c == '\\')
		return (handle_backslash_escape(p));
	return (c);
}

static int compute_size(char *ptr)
{
	char *p;
	int c;

	p = ptr;
	while ((c = *p++) && c != ':' && c != '\n')
		;
	return (p - ptr + 1);
}

static char *copy_value(char *ptr, char *ret, char **area)
{
	char *p;
	char *r;
	int c;

	p = ptr;
	r = ret;
	while ((c = *p) && c != ':' && c != '\n')
		*r++ = process_escape_char(&p);
	*r = '\0';
	if (area)
		*area = r + 1;
	return (ret);
}

char *tgetst1(char *ptr, char **area)
{
	char *ret;
	int size;

	if (!ptr)
		return (NULL);
	if (!area)
	{
		size = compute_size(ptr);
		ret = (char *)xmalloc(size);
	}
	else
		ret = *area;
	return (copy_value(ptr, ret, area));
}