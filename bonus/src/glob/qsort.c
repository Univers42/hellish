/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   qsort.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:55 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:55 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "../libft/libft.h"

void	swap_sort_point(char **p1, char **p2)
{
	char	*t;

	t = *p1;
	*p1 = *p2;
	*p2 = t;
}

int	partition(t_vec *vec, int low, int high)
{
	char	**arr;
	int		i;
	int		p_idx;
	int		j;

	ft_assert(vec->ctx != NULL);
	arr = (char **)vec->ctx;
	p_idx = (high - low) / 2 + low;
	swap_sort_point(&arr[low], &arr[p_idx]);
	i = low;
	j = high;
	while (i < j)
	{
		while (ft_strcmp(arr[i], arr[low]) < 0 && i <= high - 1)
			i++;
		while (ft_strcmp(arr[j], arr[low]) >= 0 && j >= low + 1)
			j--;
		if (i < j)
			swap_sort_point(&arr[i], &arr[j]);
	}
	return (i);
}

void	quicksort_inner(t_vec *vec, int low, int high)
{
	int	p;

	if (low < high)
	{
		p = partition(vec, low, high);
		quicksort_inner(vec, low, p - 1);
		quicksort_inner(vec, p + 1, high);
	}
}

void	ft_quicksort(t_vec *vec)
{
	if (vec->len > 1)
		quicksort_inner(vec, 0, vec->len - 1);
}
