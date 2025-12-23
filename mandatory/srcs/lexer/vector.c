/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vector.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 17:36:20 by alcacere          #+#    #+#             */
/*   Updated: 2025/12/23 20:59:47 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "inlexer.h"

t_tok_vec	*vector_init(size_t initial_capacity)
{
	t_tok_vec	*vector;

	vector = malloc(sizeof(t_tok_vec));
	if (vector == NULL)
		return (NULL);
	vector->tokens = malloc(sizeof(t_toke *) * initial_capacity);
	if (vector->tokens == NULL)
	{
		free(vector);
		return (NULL);
	}
	vector->capacity = initial_capacity;
	vector->count = 0;
	return (vector);
}

int	vector_append(t_tok_vec *vector, t_toke *token)
{
	size_t	new_capacity;
	t_toke	**new_tokens;

	if (!vector || !token)
		return (-1);
	if (vector->count == vector->capacity)
	{
		new_capacity = vector->capacity * 2;
		if (new_capacity == 0)
			new_capacity = 8;
		new_tokens = malloc(new_capacity * sizeof(t_toke *));
		if (!new_tokens)
			return (-1);
		memcpy(new_tokens, vector->tokens, vector->count * sizeof(t_toke *));
		free(vector->tokens);
		vector->tokens = new_tokens;
		vector->capacity = new_capacity;
	}
	vector->tokens[vector->count] = token;
	vector->count++;
	return (0);
}

void	vector_free(t_tok_vec *vector)
{
	size_t	i;

	if (vector == NULL)
		return ;
	i = 0;
	while (i < vector->count)
	{
		if (vector->tokens[i])
		{
			d_str_free(&vector->tokens[i]->value);
			free(vector->tokens[i]);
		}
		i++;
	}
	free(vector->tokens);
	free(vector);
}
