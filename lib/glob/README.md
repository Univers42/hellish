# Glob lib
This lib contain directories and files that specify globbing and pattern matching subsystem. 

> globbing referes to filename pattern matching using wildcards like `*`, `?`, and bracket expresions [abc]

# Features
- Standard globbing: Wildcars `*`, `?`), bracket expressions
- Extended globbing: ksh-like patterns @(foo|bar), !foo, +(foo), etc..
- Multibyte/Unicode support: Handles wilde character
- POSIX compliance: supports POSIX bracket expresssion like [[:alpha:]], [.newline.],[=a=]
- Case folding: Optional case-insensitive matching.
- Directory-specific matching: Flags to match only directories, mark directories, etc..
- Recursive globiing: support for `**` to match files recursively

# HOW IT WORKS
The globbing engine scans directories, matches filenames against patterns using the matching engin and return lists
of matches

# Why it's a lib ?
it implements filename pattern matching (globbing) using wildcard and extended patterns, which is  acommon need in shell
and file utilisies.
- The code is modular and portable, so it can be reused in any other porjets of (shells, scripting langues, file managers)
that need globbing at the end of the day. 

> It's adaptability make it adaptable and completeness make it useful for other porjects beyond shell !c
