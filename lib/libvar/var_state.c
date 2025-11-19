#include "libvar.h"

// Add extern declarations for callbacks and memory functions
extern void changemail(const char *);
extern void changepath(const char *);
extern void getoptsreset(const char *);
extern void sethistsize(const char *);
extern void *ft_memcpy(void *dst, const void *src, size_t n);
extern void *ft_memset(void *s, int c, size_t n);

static const char	g_defpathvar[] = \
	"PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin";
static char			g_defifsvar[] = "IFS= \t\n";
static char			g_defoptindvar[] = "OPTIND=1";

static int	init_varinit_pt2(struct s_var_state *state, int i)
{
	state->varinit[i++] = (struct s_var){NULL,
		VSTRFIXED | VTEXTFIXED, "PS1=$ ", 0};
	state->varinit[i++] = (struct s_var){NULL,
		VSTRFIXED | VTEXTFIXED, "PS2=> ", 0};
	state->varinit[i++] = (struct s_var){NULL,
		VSTRFIXED | VTEXTFIXED, "PS4=+ ", 0};
	state->varinit[i++] = (struct s_var){NULL,
		VSTRFIXED | VTEXTFIXED, state->defoptindvar, getoptsreset};
#ifdef WITH_LINENO
	state->varinit[i] = (struct s_var){NULL,
		VSTRFIXED | VTEXTFIXED, state->linenovar, 0};
	state->vlineno_ptr = &state->varinit[i];
	i++;
#endif
#ifndef SMALL
	state->varinit[i++] = (struct s_var){NULL,
		VSTRFIXED | VTEXTFIXED | VUNSET, "TERM\0", 0};
	state->varinit[i++] = (struct s_var){NULL,
		VSTRFIXED | VTEXTFIXED | VUNSET, "HISTSIZE\0", sethistsize};
#endif
	return (i);
}

static void	init_varinit_array(struct s_var_state *state)
{
	int	i;

	i = 0;
	state->vlineno_ptr = NULL;
#if ATTY
	state->varinit[i++] = (struct s_var){NULL,
		VSTRFIXED | VTEXTFIXED | VUNSET, "ATTY\0", 0};
#endif
	state->varinit[i++] = (struct s_var){NULL,
		VSTRFIXED | VTEXTFIXED, state->defifsvar, 0};
	state->varinit[i++] = (struct s_var){NULL,
		VSTRFIXED | VTEXTFIXED | VUNSET, "MAIL\0", changemail};
	state->varinit[i++] = (struct s_var){NULL,
		VSTRFIXED | VTEXTFIXED | VUNSET, "MAILPATH\0", changemail};
	state->varinit[i++] = (struct s_var){NULL,
		VSTRFIXED | VTEXTFIXED, state->defpathvar, changepath};
	i = init_varinit_pt2(state, i);
	state->varinit_size = i;
}

t_var_state	*get_var_state(void)
{
	static t_var_state	state;
	static int			is_initialized;

	is_initialized = 0;
	if (!is_initialized)
	{
		state.defpathvar = g_defpathvar;
		state.defifsvar = g_defifsvar;
		state.defoptindvar = g_defoptindvar;
		ft_memcpy(state.linenovar, "LINENO=", 7);
		state.linenovar[7] = '\0';
		init_varinit_array(&state);
		ft_memset(state.oplist, 0, NOPTS);
		is_initialized = 1;
	}
	return (&state);
}

void libvar_reset_state(void)
{
	// Remove unused variables to avoid warnings/errors
	// t_var   *vp;
	// t_var   **vpp;
	// t_var   *next;

	// if (!g_is_initilized)
	//     get_var_state();
	// vpp = g_state.vartab;
	// ...reset logic if needed...
}
