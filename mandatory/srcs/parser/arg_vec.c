#include "parser.h"

void	arg_vec_init(t_arg_vec *vec)
{
	if (!vec)
		return ;
	vec->args = NULL;
	vec->count = 0;
	vec->capacity = 0;
}

int	vec_add_arg(t_arg_vec *vec, char *arg)
{
	size_t	new_capacity;
	char	**new_args;

	if (!vec || !arg)
		return (1);
	if (vec->count == vec->capacity)
	{
		new_capacity = vec->capacity * 2;
		if (new_capacity == 0)
			new_capacity = 8;
		new_args = malloc(new_capacity * sizeof(char *));
		if (!new_args)
			return (1);
		memcpy(new_args, vec->args, vec->count * sizeof(char *));
		free(vec->args);
		vec->args = new_args;
		vec->capacity = new_capacity;
	}
	vec->args[vec->count] = arg;
	vec->count++;
	return (0);
}

void	free_arg_vec(t_arg_vec *vec)
{
	size_t	i;

	if (!vec)
		return ;
	i = 0;
	while (i < vec->count)
	{
		free(vec->args[i]);
		i++;
	}
	free(vec->args);
	vec->args = NULL;
	vec->count = 0;
	vec->capacity = 0;
}