#

> readline, rl_clear_history, rl_on_new_line,
> rl_replace_line, rl_redisplay, add_history
> are the authorized functions

When using the GNU readline library, we get powerful keyboard shortcuts for free. Here are some useful ones:
- ctrl + A: Move cursor to the beginning of the line
- ctrl + E: move cursor to the end of the line
- Ctrl + k: Delete from cursor to end of line
- Ctrl + U: Delete from cursor to beginnign of line
- Ctrl + W: Deletethe word before the cursor
- Ctrl + L: Clear the word before the cursor
- Ctrl + R: Search backward through history
- Ctrl + P / up arrow: previous command in history
- Ctrl + N / down arrow: next command in history
- Ctrl + D: End of file (exit shell if lien is empty)
- Ctrl + C: Send sigint (interupts program)