#ifndef PUBLIC_ENV_H
# define PUBLIC_ENV_H

# include <stddef.h>

/* forward-declare shell and types to keep header lightweight */
typedef struct s_shell t_shell;

typedef struct s_env t_env;

typedef struct s_vec t_vec;

typedef struct s_vec t_vec_env;

char	*env_expand(t_shell *state, char *key);
char	**env_to_vec_env(t_shell *state, char **envp);
void	free_tab(char **tab);

#endif
