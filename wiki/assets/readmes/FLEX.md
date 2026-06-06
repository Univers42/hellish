# Grammar

There are two main approaches
|approach|description| examples|
|---|---|----|
Manual FSM (finite state machine) | we write the state by hand switching/case, pointer walks, etc. | good for shells
lexer generator (flex) | we write regex-like grammar `flex` generates c code for us | overkill for sh42

The grammar in FSM desing is just a huge list of enum, that will be used as reference for
finding the type of each words we find. IN other words, it's just a more fancy split function.

