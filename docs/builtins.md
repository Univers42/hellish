# Exit builtin
The exit builtin is a special command that terminates the shell.

It takes an optional numeric argument as the shell's exit status.

```bash
$ exit 0        # exit with status 0 (success)
$ exit 1        # exit with status 1
```

when out shell receives the `exit` command, it should immediately terminate with the specified exit code.

# Type builtin
- The type builtin is usd to determin how a command would be interpreted if it were used. 
it checks whether a command is a builting, an executable file, or unrecognized

```bash

$ type echo
echo is a shell builtin
$ type exit
exit is a shell builtin
$ type invalid
invalid: not found
```

# Firstly we will handle tow edge cases :
- For builting commands
- For unrecognized command that don't amtch any builting

