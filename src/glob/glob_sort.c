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

int				is_letter(unsigned char c);
int				is_digit_char(unsigned char c);
int				is_alnum_char(unsigned char c);
unsigned char	to_lower(unsigned char c);
int				ft_strcoll(const char *s1, const char *s2);

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
		while (ft_strcoll(arr[i], pivot) < 0)
			i++;
		while (ft_strcoll(arr[j], pivot) > 0)
			j--;
		if (i <= j)
			ft_swap(&arr[i++], &arr[j--], sizeof(char *));
	}
	(glob_sort_inner(arr, low, j), glob_sort_inner(arr, i, high));
}

void	glob_sort(t_vec *args)
{
	if (args->len > 1)
		glob_sort_inner((char **)args->ctx, 0, (int)args->len - 1);
}
