/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   var.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alcacere <alcacere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 19:43:36 by alcacere          #+#    #+#             */
/*   Updated: 2025/11/04 21:28:21 by alcacere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VAR_H
# define VAR_H

# define VTABSIZE 39

// ...existing code...
#ifndef VAR_H
# define VAR_H

# include <limits.h>

# define VTABSIZE 39

typedef struct localvar_list {
    struct localvar_list *next;
    struct localvar *lv;
}	t_local_var_list;

/* ...existing code... */

/*
 * Group globals into a single structure to avoid scattered global variables.
 */
typedef struct s_gvar {
    char	defpathvar[sizeof("PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin")];
    char	defifsvar[sizeof("IFS= \t\n")];
    char	defoptindvar[sizeof("OPTIND=1")];
    int		lineno;
    /* space for "LINENO=" + decimal digits for int */
    char	linenovar[sizeof("LINENO=") + (sizeof(int) * CHAR_BIT / 3) + 2];
    struct localvar_list *localvar_stack;
}	t_gvar;

/* single instance, defined in var.c */
extern t_gvar gvar;

/* ...existing code... */

void	export_names(char **aptr, int flag);
int	exportcmd(int argc, char **argv);
void	var_init(void);

#endif
// ...existing code...

// ...existing code...
#include "var.h"

/* define the grouped globals and initialize with the same values */
t_gvar gvar = {
    .defpathvar = "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
    .defifsvar = "IFS= \t\n",
    .defoptindvar = "OPTIND=1",
    .lineno = 0,
    .linenovar = "LINENO=",
    .localvar_stack = NULL
};

/* Example: use gvar members where the separate globals used to be used. */
/* Some macros in var.h depend on the order, add new variables to the end. */
struct var varinit[] = {
#if ATTY
    { 0,	VSTRFIXED|VTEXTFIXED|VUNSET,	"ATTY\0",	0 },
#endif
    { 0,	VSTRFIXED|VTEXTFIXED,		gvar.defifsvar,	changeifs },
    { 0,	VSTRFIXED|VTEXTFIXED|VUNSET,	"MAIL\0",	changemail },
    { 0,	VSTRFIXED|VTEXTFIXED|VUNSET,	"MAILPATH\0",	changemail },
    { 0,	VSTRFIXED|VTEXTFIXED,		gvar.defpathvar,	changepath },
    { 0,	VSTRFIXED|VTEXTFIXED,		"PS1=$ ",	0 },
    { 0,	VSTRFIXED|VTEXTFIXED,		"PS2=> ",	0 },
    { 0,	VSTRFIXED|VTEXTFIXED,		"PS4=+ ",	0 },
    { 0,	VSTRFIXED|VTEXTFIXED|VNOFUNC,	gvar.defoptindvar,	getoptsreset },
#ifdef WITH_LINENO
    { 0,	VSTRFIXED|VTEXTFIXED,		gvar.linenovar,	0 },
#endif
#ifndef SMALL
    { 0,	VSTRFIXED|VTEXTFIXED|VUNSET,	"TERM\0",	0 },
    { 0,	VSTRFIXED|VTEXTFIXED|VUNSET,	"HISTSIZE\0",	sethistsize },
#endif
};

typedef struct s_local_var_list
{
	struct	local_var_list	*next;
	struct	localvar		*lv;
}	t_local_var_list;

/**
 * Fumnction to be called when 
 */
typedef struct s_var
{
	int			flags;					// flags are defined above
	const char	*text;					// name=value
	void		(*func)(const char *);	//called when the var gets set/unset
	struct var	*next;					// next entry in hash list
}	t_var;

typedef struct s_gvar
{
	int lineno;
	char	defifsvar[] = "IFS= \t\n";
	char	linenovevar
}t_gvar var
int		varcmp(const char *p, const char *q);
t_var	**hashvar(const char *p);
void    initvar(void);

/**
 * export_names
 *
 * Process a NULL-terminated array of variable specification strings and apply
 * the given flag to each variable. Each element of aptr is either "name" or
 * "name=value".
 *
 * @param {Array.<string>} aptr  NULL-terminated array of variable strings
 * @param {number}         flag  Flag to apply (e.g. VEXPORT or VREADONLY)
 *
 * Behavior:
 *  - If an entry contains '=', calls var_set(name, value, flag).
 *  - Otherwise, if the variable exists, ORs vp->flags with flag.
 *  - If the variable does not exist, calls var_set(name, NULL, flag).
 *
 * @returns {void}
 */
void	export_names(char **aptr, int flag);

/**
 * exportcmd
 *
 * Builtin command handler for "export" and "readonly".
 *
 * @param number       argc	Number of arguments.
 * @param Array argv	Argument vector; 
 * @note argv[0][0] determines whether
 *                              this is "readonly" ('r') or "export".
 *
 * Determines the appropriate flag (VREADONLY or VEXPORT), parses the "-p"
 * option via nextopt("p"), and either processes supplied names via
 * export_names or shows variables with var_show().
 *
 * @return number Exit status (0 on success).
 */
int	exportcmd(int argc, char **argv);

/**
 * var_init 
 * 
 * initialiaze the shell's variable subsystem
 * @note creates/initializes the variable smbol table(s) and
 * inserts the built-in vaiable entries (the varinit arrya) into
 * the variable hash or lists;
 * set up fixed/default variables (IFS, OPTIND, LINENO, etc.) and their flags
 * it is used during hsell startup to import the environemnt and call any
 * setup hooks
 * maeks a final adjustement for te prompt
*/
void	var_init(void);

#endif