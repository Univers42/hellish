#include "glob_private.h"

const struct s_classes classes[] = {
	{"[:alnum:]", 9, "0123456789ABCDEFGHIJKLMNO"
					 "PQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"},
	{"[:alpha:]", 9, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					 "abcdefghijklmnopqrstuvwxyz"},
	{"[:digit:]", 9, "0123456789"},
	{"[:lower:]", 9, "abcdefghijklmnopqrstuvwxyz"},
	{"[:upper:]", 9, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"},
	{"[:space:]", 9, " \t\n\r\f\v"},
	{"[:blank:]", 9, " \t"},
	{"[:punct:]", 9, "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"},
	{"[:xdigit:]", 10, "0123456789ABCDEFabcdef"},
	{"[:cntrl:]", 9, "\001\002\003\004\005\006\007\010"
					 "\011\012\013\014\015"
					 "\016\017\020\021\022\023\024\025\026\027\030"
					 "\031\032\033\034\035\036\037\177"},
	{"[:graph:]", 9, "!\"#$%&'()*+,-./0123456789:;<=>?@"
					 "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^"
					 "_`abcdefghijklmnopqrstuvwxyz{|}~"},
	{"[:print:]", 9, " !\"#$%&'()*+,-./0123456789:;"
					 "<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]"
					 "^_`abcdefghijklmnopqrstuvwxyz{|}~"},
	{NULL, 0, NULL}};
