# Token recognition
`Token recognition` = shell scanning the input and identifying chunks (tokens)
- it follows fixed POSIX rules:
- Words = sequences of non-special chars.
- Operators = `|`, `;`,`>`,`<`, `||`, `&&`, etc.
- Quoting changes how tokens end.
- `<<word` = special token type `io_here`

- two modes either ordinary token recognition and processing of here-documents
- The shell grammar defines token types like `word` `operator` `io_redirect`,etc-

When the shell parser sees something lie `<<EOF`, it marks that as an I/O redirection token for a here-document.
Then after the next newline, the shell treats the following lines as teh body of that here-documents, until it finds the delimiter like `EOF`

when it encounters 
```c
cat <<EOF
hello
EOF

# token recognition result
[
	WORD("cat"),
	IO_HERE("EOF"),
	NEWLINE,
	HERE_BODY("hello\n")
]
```
## Example walkthough
input:
```bash
cat << EOF | grep "foo" # comment
```

```bash
[
	("WORD", "cat"),
	("OPERATOR", "<<"),
	("WORD", "EOF"),
	("WORD", "grep"),
	("WORD", "foo")
]
```

## Strategy when working with tokens (there is differenct ctx to repeat for each verif)
- Inside here-documents:
- Inside command substritution and arithmetic expansion:
- Insdie double quuotes
- Inside single quotes
- Outside of quotes:
- parameter expansion, inside `${...}`

| #     	| ctx                                           	| Why it matters / what to verify 																																							|
| ------	| --------------------------------------------------	|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------  |
| **1**		|**here documents** -- `< > >> << <& >&`				| 
| **2**		|** Inside quoting** -- `[b\|d\|s]quote`				|
| **3**		| **outside of quotes**									|
| **4** 	| **Parameter expansion** — inside `${...}`         	|	 Verify how quoting, escapes, and nested expansions work (e.g. `${var\}` vs `${var:-"foo"}` vs `${#var}`).Also test quoting inside pattern removals `${var%"pattern"}`.                  	|
| **5** 	| **Command word vs arguments**                     	| In POSIX shells, the *first token* (the command name) is recognized before field splitting, alias expansion, or pathname expansion. Verify how escapes affect the *command position* token. |
| **6** 	| **Redirection and assignment ctxs**           	| Test escaping in `VAR=value`, `echo foo\>bar`, `echo >file\ name`, etc. Lexer must distinguish operators like `>`, `>>`, `<&`, etc.                                                         |
| **7**		| **Alias and reserved word recognition**           	| Quoting and escaping can suppress recognition. For instance, `\if` is a literal command, not a reserved word. Verify that escapes disable keyword parsing.                                  |
| **8**		| **Pathname expansion (globbing)**                 	| Backslash can disable `*`, `?`, and `[` expansion; quoting also prevents it. Verify how these interact before tokenization.                                                                 |
| **9**		| **Tilde expansion**                               	| Occurs only in unquoted, unescaped ctxs at the start of a word (`~/foo`). Test that `\~` or `echo "~"` disables expansion.                                                              |
| **10**	| **Field splitting (word splitting)**              	| After expansions, unquoted results are split by IFS. Verify that quoting or escaping prevents splitting (`echo "$var"` vs `echo $var`).                                                     |
| **11**	| **Arithmetic vs command substitution nesting**    	| Ensure correct layering: e.g. `"$(( $(echo 1)+1 ))"`, verifying how tokens are re-lexed inside.                                                                                             |
| **12**	| **Comments**                                      	| Test that `#` starts a comment only when not quoted or escaped. Verify `echo \#notcomment` and `echo '#stillnotcomment'`.                                                                   |
| **13**	| **Continuation lines and whitespace handling**    	| Verify that `\<newline>` joins lines, doesn’t insert whitespace, and can appear after operators or quotes.                                                                                  |
| **14**	| **Process substitution (in Bash/Zsh)**            	| `<(command)` and `>(command)` — verify how escapes and quoting affect parsing. (Non-POSIX but common.)                                                                                      |
| **15**	| **Array or associative array ctxs (Bash/Zsh)**	| `arr=(a\ b "c d")` — escaping and quoting rules apply during array word splitting.                                                                                                          |
| **16**	| **Subshells / grouping**                          	| `( echo "hi" )` vs `\( echo hi \)` — verify recognition of grouping tokens when escaped.                                                                                                    |


For each of those ctx, we have to verify:
1. lexical removal -- What characters are stripped (quotes, backslashes, line continuations)
2. Expansions phase -- What expansions occur (parameter, command arithmetic, arithmetic, pathname, tilde)
3. field splitting --  does it happen or is it suppressed by quoting ?
4. Command lookup -- after toikens are formed, what command and argments does the shell actually execute?
5. Error classification -- distinguish parser errors (syntax) vs execution errors (eg. "command not found")

notice that `<<` is recognized as an operator (technicall `io_hre`)
the `# comment` part is ignored after the `#`
Quotes `foo` becomes a signle **WORD** token

The Way I see it this pseudo code respond briefly on how the code should behave

# !POSIX compliance is way more nuanced than people think
The posix shell's lexical rules aren't just:
> "split  on whitespace and respect quotes"

They're ctx sensitive, meaning the interpretation of a character depends on:
- The current quoting state
- What came before (operator, redirect, etc..)
- Whether the parser expects an assignment or a command.
- and sometimes even on expansion rulse that happen after lexing.

So implementing that correctly requires :
1. Recursive parsing ctx
2. Lookhead into expansion phases
3. Grammar feedback into lexing decisions

A simple hand-rolled lexer like the one above can't do all that.

|           **POSIX rule**  |        **WHAT POSIX SAYS**|
|----------                 |---------------            |
|**quotes join tokens**     |1.1|
|**escape rules**           |1.2| 
|**line continuation**      |1.3|
|**here-doc delimiters**    |1.4|
|**operators**              |1.5|
|**Comments**               |1.6|
|**Reserved words**         |1.7|
|**I/O numbers**            |1.8|
|**Here-doc scheduling**	|1.9|

## 1.1 Quotes join tokens
### Escape characters
|Escape sequences| Meaning effects|
|----------------|----------------|
|`\<newline>`|line continuation - both the backslash and the newlines are removed|
|`\\`, `\$`, `\*`, `\?`, `\|`, `\&`, `\`;, `\(`, `\)`, `\<`, `\>`, `\'`, `\"`, `\<bq>`, etc.| Removes the special syntactic meaning of that character - makes it literal
|`\`followed by any other character | that character is taken literally (no transformation)|
|`/`|Escaped space -- keeps teh space from  separating tokens|



|Quote type|Meaning|Notes|
|----------|-------|-----|
|No quotes|Words split by unquoted space/tabs/newlines. Backlash espaces next char|`\` can escape anything.
|Single quotes|everything inside is literal|No escaping inside; even `\` and `$` are literal|
|Double quotes|Most characters literal, except `$`, backquote, `\` , `"` |backlash escapes only a few things|


```bash
[DEBUG] $> echo "hello\n"

=== LEXER DEBUG OUTPUT ===
 TOKEN_IDENTIFIER            | line 1 | echo
 TOKEN_DOUBLE_QUOTED_STRING  | line 1 | hello\n
 TOKEN_EOF                   | line 1 | 
=== END DEBUG OUTPUT ===
[DEBUG] $> ^C
```
```bash

❯ echo hello\ world
hello world
❯ echo hello\
> world  
helloworld
❯ echo $HOME
/home/dlesieur
❯ echo \$HOME
$HOME
❯ echo '$HOME'
$HOME
❯ echo \
> $HOME
/home/dlesieur
❯ echo one\ two\&three
one two&three
❯ echo 'can'\''t' | cat -e
can't$
❯ echo "a\"b" | cat -e
a"b$
❯ echo "a\$b" | cat -e
a$b$
❯ echo foo\nbar
foonbar
❯ echo "foo\nbar"
foo
bar
❯ echo 'foo\nbar'
foo
bar
❯ echo 'foo\nbar'
foo
bar
❯ echo 'foo\${HOME}bar'
foo\${HOME}bar
❯ echo 'foo\nbar'
foo
bar
❯ echo 'foo\${HOME}bar'
foo\${HOME}bar
❯ echo 'foo${HOME}bar'
foo${HOME}bar
❯ echo foo${HOME}bar
foo/home/dlesieurbar
❯ echo "x$(echo y)"
xy
❯ echo "x$(echo \ny)"
xny
❯ echo 'x$(echo \ny)'
x$(echo 
y)
❯ echo x$(echo \ny)
xny
❯ echo \x$(echo \ny)
xny
❯ echo \x\$(echo \ny)
zsh: missing end of string
```


### Single quotes
Enclosing characters in single quotes shall preserver the literal value of each character within the the single quotes. A single quotes cannot occur withing single-quotes.

```bash
echo 'hello $(USER)'
dlesieur@c1r15s1:/sgoinfre/students/dlesieur/dlesieur/42/sh42$ echo "hello $LOGNAME"
hello dlesieur
dlesieur@c1r15s1:/sgoinfre/students/dlesieur/dlesieur/42/sh42$ echo 'hello $LOGNAME'
hello $LOGNAME
dlesieur@c1r15s1:/sgoinfre/students/dlesieur/dlesieur/42/sh42$ echo "hello \$LOGNAME"
hello $LOGNAME
dlesieur@c1r15s1:/sgoinfre/students/dlesieur/dlesieur/42/sh42$ echo "hello\' $LOGNAME"
hello\' dlesieur
dlesieur@c1r15s1:/sgoinfre/students/dlesieur/dlesieur/42/sh42$ echo 'hello\' $LOGNAME'
> '
hello\ dlesieur

```
## What is the difference between single quote and bacslash then ?

### Double quotes

```bash
❯ name="Dylan"
❯ echo "Hello $name!"
dquote>   
❯ echo "Hello $name"
Hello Dylan
❯ echo "TODAY is `date +%A`"
TODAY is Tuesday
❯ echo "She said: \"Hello\""
She said: "Hello"
❯ echo "C:\Users\Dylan"
C:sers\Dylan
❯ user="dlesieur"
echo "User $user is running \`whoami\`"

User dlesieur is running `whoami`
❯ user="dlesieur"
echo "User $user is running `whoami`"

User dlesieur is running dlesieur

```
## 1.2 Escape rules
## 1.3 Line Continutation
## 1.4 Here-Doc delimiters
## 1.5 Operators
## 1.6 Comments
## 1.7 Reserved Words
## 1.8 I/O numbers
## 1.9 Scheduling


## Token recognition

The shell should read its input in **terms of lines**

---

# Vocabulary
**literal value**: raw datas types, `int lit = 8` --> literal value = 8
**expansion**: The expansion is the representation of what's behind the shortcut variable literal

