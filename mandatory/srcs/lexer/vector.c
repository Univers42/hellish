/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vector.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alcacere <alcacere@student.42madrid.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 17:36:20 by alcacere          #+#    #+#             */
/*   Updated: 2025/12/19 17:36:26 by alcacere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

t_token_vector  *vector_init(size_t initial_capacity)
{
    t_token_vector  *vector;

    vector = malloc(sizeof(t_token_vector));
    if (vector == NULL)
        return (NULL);
    vector->tokens = malloc(sizeof(t_token *) * initial_capacity);
    if (vector->tokens == NULL)
    {
        free(vector);
        return (NULL);
    }
    vector->capacity = initial_capacity;
    vector->count = 0;
    return (vector);
}

int vector_append(t_token_vector *vector, t_token *token)
{
    size_t  new_capacity;
    t_token **new_tokens;

    if (vector == NULL || token == NULL)
        return (-1);
    if (vector->count == vector->capacity)
    {
        new_capacity = vector->capacity * 2;
        if (new_capacity == 0)
            new_capacity = 1;
        new_tokens = malloc(new_capacity * sizeof(t_token *));
        if (new_tokens == NULL)
            return (-1);
        memcpy(new_tokens, vector->tokens, vector->count * sizeof(t_token *));
        free(vector->tokens);
        vector->tokens = new_tokens;
        vector->capacity = new_capacity;
    }
    vector->tokens[vector->count] = token;
    vector->count++;
    return (0);
}

void    vector_free(t_token_vector *vector)
{
    size_t  i;

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
