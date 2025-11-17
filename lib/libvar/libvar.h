#ifndef LIBVAR_H
# define LIBVAR_H

/*
 * External headers required for types used in the prototypes.
 */
# include <stddef.h>
# include <inttypes.h>

/*
 * Hash table size
 */
# define VTABSIZE 39

/*
 * Variable flags
 */
# define VEXPORT		0x01
# define VREADONLY	0x02
# define VSTRFIXED	0x04
# define VTEXTFIXED	0x08
# define VSTACK		0x10
# define VUNSET		0x20
# define VNOFUNC		0x40
# define VNOSET		0x80
# define VNOSAVE		0x100

/*
 * Structure Definitions
 */

/**
 * @brief Represents a single variable in the hash table.
 */
typedef struct s_var
{
	struct s_var	*next;
	int				flags;
	const char		*text;
	void			(*func)(const char *);
}					t_var;

/**
 * @brief Represents a saved variable state for 'local' command.
 */
typedef struct s_localvar
{
	struct s_localvar	*next;
	t_var				*vp;
	int					flags;
	const char			*text;
}						t_localvar;

/**
 * @brief A node in the stack of local variable scopes.
 */
typedef struct s_localvar_list
{
	struct s_localvar_list	*next;
	t_localvar				*lv;
}							t_localvar_list;

/**
 * @brief Opaque structure for the singleton state.
 * The full definition is private to 'var_state.c'.
 */
typedef struct s_var_state	t_var_state;

/*
 * Core State Singleton
 */

/**
 * @brief Gets the singleton instance of the variable state.
 * @return A pointer to the unique t_var_state struct.
 */
t_var_state		*get_var_state(void);

/*
 * Core Variable Functions
 */

/**
 * @brief Sets or unsets a variable.
 * @param name The name of the variable to set.
 * @param val The value to set. If NULL, the variable is unset.
 * @param flags Flags to be OR'ed with the variable's flags (e.g., VEXPORT).
 * @return A pointer to the variable structure.
 */
t_var			*setvar(const char *name, const char *val, int flags);

/**
 * @brief Sets a variable to an integer value.
 * @param name The name of the variable to set.
 * @param val The integer (intmax_t) value.
 * @param flags Flags to be OR'ed with the variable's flags.
 * @return The integer value that was set.
 */
intmax_t		setvarint(const char *name, intmax_t val, int flags);

/**
 * @brief Sets a variable from a "name=value" string.
 * @param s The string in "name=value" format.
 * @param flags Flags to be OR'ed with the variable's flags.
 * @return A pointer to the variable structure.
 */
t_var			*setvareq(char *s, int flags);

/**
 * @brief Looks up the value of a variable.
 * @param name The name of the variable.
 * @return The value of the variable as a string, or NULL if not set.
 */
char			*lookupvar(const char *name);

/**
 * @brief Looks up the value of a variable and converts it to an integer.
 * @param name The name of the variable.
 * @return The integer value (intmax_t), or 0 if not set or not a number.
 */
intmax_t		lookupvarint(const char *name);

/**
 * @brief Unsets the specified variable.
 * @param s The name of the variable to unset.
 */
void			unsetvar(const char *s);

/*
 * Built-in Command Implementations
 */

/**
 * @brief Implementation of the 'export' and 'readonly' built-in commands.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success.
 */
int				exportcmd(int argc, char **argv);

/**
 * @brief Implementation of the 'local' built-in command.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success.
 */
int				localcmd(int argc, char **argv);

/**
 * @brief Implementation of the 'unset' built-in command.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success.
 */
int				unsetcmd(int argc, char **argv);

/**
 * @brief Prints variables in lexicographic order, suitable for 'set'.
 * @param prefix A string to print before each variable (e.g., "export").
 * @param on A bitmask of flags that must be ON.
 * @param off A bitmask of flags that must be OFF.
 * @return 0 on success.
 */
int				showvars(const char *prefix, int on, int off);

/*
 * Local Scope (Function) Management
 */

/**
 * @brief Makes a variable local to the current function scope.
 * @param name The name of the variable (can be "name=value").
 * @param flags Additional flags to apply.
 */
void			mklocal(char *name, int flags);

/**
 * @brief Pushes a new local variable scope onto the stack.
 * @param push 1 to push a new scope, 0 to just return the current top.
 * @return The top of the local variable stack (before pushing, if push=1).
 */
t_localvar_list	*pushlocalvars(int push);

/**
 * @brief Pops the current local variable scope from the stack.
 */
void			poplocalvars(void);

/**
 * @brief Unwinds the local variable stack down to a specific level.
 * @param stop The stack level to stop at (returned by a previous pushlocalvars).
 */
void			unwindlocalvars(t_localvar_list *stop);

/*
 * List & Helper Functions
 */

/**
 * @brief Generates a list of variables matching flag conditions.
 * @param on A bitmask of flags that must be ON.
 * @param off A bitmask of flags that must be OFF.
 * @param end A pointer that will be set to the end of the returned list.
 * @return An array of strings ("name=value"), NULL-terminated.
 */
char			**listvars(int on, int off, char ***end);

/**
 * @brief (Was static) Hashes a variable name.
 * @param p The variable name.
 * @return A pointer to the bucket in the state->vartab.
 */
t_var			**var_hash(const char *p);

/**
 * @brief (Was static) Finds a variable in a hash bucket.
 * @param vpp Pointer to the hash bucket (from var_hash).
 * @param name The name of the variable to find.
 * @return A pointer to the variable's slot in the list.
 */
t_var			**var_find(t_var **vpp, const char *name);

/**
 * @brief (Was 'varcmp') Compares two variable names (up to '=' or '\0').
 * @param p First string (must end in '=' or '\0').
 * @param q Second string (must end in '=' or '\0').
 * @return An integer <, ==, or > 0.
 */
int				libvar_varcmp(const char *p, const char *q);

/**
 * @brief (Was 'vpcmp') Comparison function for qsort (uses libvar_varcmp).
 * @param a Pointer to the first string.
 * @param b Pointer to the second string.
 * @return An integer <, ==, or > 0.
 */
int				var_vpcmp(const void *a, const void *b);

/*
 * TEST UTILITY: Do not use in production.
 * Resets the singleton state for unit testing.
 */
void	libvar_reset_state(void);

#endif
