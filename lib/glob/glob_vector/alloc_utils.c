#define _POSIX_C_SOURCE 200809L
#include <alloca.h>
#include <stdlib.h>
#include "glob_vector.h"

void *alloca_safe(size_t n)
{
	if (n <= 1024)
		return alloca(n);
	return malloc(n);
}

t_list *make_globval_node(t_list **firstmalloc, int *nalloca)
{
	t_list *node;

	if (*nalloca < ALLOCA_MAX)
	{
		node = (t_list *) alloca(sizeof(struct globval));
		*nalloca += sizeof(struct globval);
		return node;
	}
	node = (t_list *) malloc(sizeof(struct globval));
	if (node && *firstmalloc == NULL)
		*firstmalloc = node;
	return node;
}

int push_name_node(t_list **lastlink, t_list **firstmalloc,
		   int *nalloca, char *name)
{
	t_list *node;

	node = make_globval_node(firstmalloc, nalloca);
	if (node == NULL)
		return -1;
	node->next = *lastlink;
	*lastlink = node;
	node->name = name;
	return 0;
}

void free_globvals_on_error(t_list *lastlink, t_list *firstmalloc)
{
	t_list *tmplink;

	while (lastlink)
	{
		if (firstmalloc && lastlink == firstmalloc)
			firstmalloc = NULL, tmplink = lastlink;
		else
			tmplink = NULL;
		if (lastlink->name)
			free(lastlink->name);
		lastlink = lastlink->next;
		if (tmplink)
			free(tmplink);
	}
}

void free_alloced_nodes(t_list *lastlink, t_list *firstmalloc)
{
	t_list *next;

	if (firstmalloc == NULL)
		return;
	while (lastlink)
	{
		next = lastlink->next;
		if (lastlink == firstmalloc)
		{
			while (lastlink)
			{
				t_list *tmp = lastlink->next;
				free(lastlink);
				lastlink = tmp;
			}
			break;
		}
		lastlink = next;
	}
}
