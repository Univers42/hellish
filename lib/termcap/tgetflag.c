/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tgetflag.c                                         :+:      :+:    :+:   */
/*                                                                            */
/* ************************************************************************** */

#include "termcap.h"

int tgetflag(const char *cap)
{
	t_tglob *g;
	char *ptr;

	g = access_glob();
	if (!g->term_entry || !cap)
		return (0);
	ptr = find_capability(g->term_entry, (char *)cap);
	return (ptr && ptr[-1] == ':');
}
