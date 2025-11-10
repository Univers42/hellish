/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tputs.c                                            :+:      :+:    :+:   */
/*                                                                            */
/* ************************************************************************** */

#include "termcap.h"

static int get_baud_speed(void)
{
	t_tglob *g;
	int i;

	g = access_glob();
	i = g->ospeed;
	if (i == 0)
		return (g->tputs_baud_rate);
	if (i > 0 && i < 19)
		return (g->speeds[i]);
	return (0);
}

static int parse_pad(char **s, int nlines)
{
	int p;

	p = 0;
	while (**s >= '0' && **s <= '9')
	{
		p += *(*s)++ - '0';
		p *= 10;
	}
	if (**s == '.')
	{
		(*s)++;
		p += *(*s)++ - '0';
	}
	if (**s == '*')
	{
		(*s)++;
		p *= nlines;
	}
	return (p);
}

static void out_str(char *s, int (*fn)(int))
{
	while (*s)
		fn(*s++);
}

static int finish_pad(int p, int speed)
{
	p *= speed;
	p += 500;
	p /= 1000;
	if (speed < 0)
		p = -p;
	else
	{
		p += 50;
		p /= 100;
	}
	return (p);
}

static void out_pad(int p, int (*fn)(int))
{
	t_tglob *g;

	g = access_glob();
	while (p-- > 0)
		fn(g->pc);
}

int tputs(const char *str, int nlines, int (*outfn)(int))
{
	int speed;
	int padcount;
	char *work;

	if (!str)
		return (-1);
	work = (char *)str;
	speed = get_baud_speed();
	padcount = parse_pad(&work, nlines);
	out_str(work, outfn);
	padcount = finish_pad(padcount, speed);
	out_pad(padcount, outfn);
	return (0);
}