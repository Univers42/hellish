/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tgetstr.c                                          :+:      :+:    :+:   */
/*                                                                            */
/* ************************************************************************** */

#include "termcap.h"

char *tgetstr(const char *cap, char **area)
{
	t_tglob *g;
	char *ptr;

	g = access_glob();
	if (!g->term_entry || !cap)
		return (NULL);
	ptr = find_capability(g->term_entry, (char *)cap);
	if (!ptr || (ptr[-1] != '=' && ptr[-1] != '~'))
		return (NULL);
	return (tgetst1(ptr, area));
}