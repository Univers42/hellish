#ifndef GLOB_VECTOR_H
# define GLOB_VECTOR_H

# include <stddef.h>
# include "glob.h"

char **glob_vector(char *pat, char *dir, int flags);

void *alloca_safe(size_t n);
t_list *make_globval_node(t_list **firstmalloc, int *nalloca);
int push_name_node(t_list **lastlink, t_list **firstmalloc,
		   int *nalloca, char *name);
void free_globvals_on_error(t_list *lastlink, t_list *firstmalloc);
void free_alloced_nodes(t_list *lastlink, t_list *firstmalloc);

int handle_empty_pat(char *dir, t_list **lastlink,
		     t_list **firstmalloc, int *nalloca,
		     unsigned int *count);
int handle_literal_pat(char *pat, size_t patlen, char *dir,
		       t_list **lastlink, t_list **firstmalloc,
		       int *nalloca, unsigned int *count);

int scan_directory_and_collect(char *pat, char *dir, int flags,
			       t_list **lastlink, t_list **firstmalloc,
			       int *nalloca, unsigned int *count,
			       int *lose, int *add_current);

#endif
