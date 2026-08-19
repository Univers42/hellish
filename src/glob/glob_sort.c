/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_sort.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/25 21:03:50 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/25 21:11:44 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"
#include <string.h>

/* In-place quicksort on an array of char* using strcoll.
   POSIX requires that glob results be sorted by the CURRENT COLLATION, which
   is byte order only in the C/POSIX locale. main() already does
   setlocale(LC_ALL, ""), so under a locale like en_US.UTF-8 bash interleaves
   case (backlog.md bench build CLAUDE.md ...) while plain byte order puts
   every capital first (CLAUDE.md ... backlog.md). Sorting with ft_strcmp
   diverged from bash in any mixed-case directory; the earlier home-grown
   ft_strcoll was worse still (a case-insensitive, non-alnum-skipping
   "natural" sort), which is presumably why it was dropped for ft_strcmp
   rather than fixed. libc's strcoll is the one that matches.
   Pivot is the middle element (median of three would be safer but the inputs
   are typically short and mostly pre-sorted, so centre pivot is fine). */
void	glob_sort_inner(char **arr, int low, int high)
{
	int		i;
	int		j;
	char	*pivot;

	if (low >= high)
		return ;
	pivot = arr[(low + high) / 2];
	i = low;
	j = high;
	while (i <= j)
	{
		while (strcoll(arr[i], pivot) < 0)
			i++;
		while (strcoll(arr[j], pivot) > 0)
			j--;
		if (i <= j)
			ft_swap(&arr[i++], &arr[j--], sizeof(char *));
	}
	(glob_sort_inner(arr, low, j), glob_sort_inner(arr, i, high));
}

/* Sort the matched pathname strings in args. A single result needs no sorting;
   two or more go through glob_sort_inner. Called from expand_word_glob after
   the directory walk, but only when should_unwind is false -- no point sorting
   a result set we're about to discard because of a signal. */
void	glob_sort(t_vec *args)
{
	if (args->len > 1)
		glob_sort_inner((char **)args->ctx, 0, (int)args->len - 1);
}
