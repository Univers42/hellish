#include <stdbool.h>
#include <ctype.h>

// Returns true if c is a valid first character for a shell variable name
bool is_var_name_p1(char c)
{
	// POSIX: first char must be alpha or '_'
	return (isalpha((unsigned char)c) || c == '_');
}

// Returns true if c is a valid subsequent character for a shell variable name
bool is_var_name_p2(char c)
{
	// POSIX: subsequent chars can be alnum or '_'
	return (isalnum((unsigned char)c) || c == '_');
}
