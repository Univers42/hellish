#ifndef PUBLIC_HELPERS_H
# define PUBLIC_HELPERS_H

# include <stdlib.h>

/* forward declare shell */
typedef struct s_shell t_shell;

char	*getpid_hack(void);
void	free_all_state(t_shell *state);
void	manage_history(t_shell *state);

#endif
