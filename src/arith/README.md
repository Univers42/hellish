# Arithmetic Expression Parser

This module implements a complete arithmetic expression evaluator for the shell, supporting C-style arithmetic with proper operator precedence, associativity, and various numeric bases.

## Overview

The arithmetic parser is designed as a self-contained API that can evaluate mathematical expressions in shell ctxs (like `$((expression))` in bash). It consists of three main components:

1. **Lexer** - Tokenizes the input string into arithmetic tokens
2. **Parser** - Builds an abstract syntax tree using recursive descent parsing
3. **Evaluator** - Computes the final result

## Architecture

### Core Components

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   Lexer     │───▶│   Parser    │───▶│ Evaluator   │
│ (Tokenizer) │    │(Syntax Tree)│    │ (Result)    │
└─────────────┘    └─────────────┘    └─────────────┘
```

### File Structure

- `eval.c` - Main evaluation entry points and string conversion
- `lexer.c` - Tokenization logic
- `parser.c` & `parser2.c` - Recursive descent parser implementation
- `helpers1.c` to `helpers8.c` - Parser functions for different precedence levels
- `set.c` - Utility functions for token setting
- `arith_private.h` - Internal API definitions

## API Usage

### Main Functions

```c
// Evaluate an arithmetic expression and return the result
long long arith_eval(t_shell *state, const char *expr, int len, bool *error);

// Evaluate and convert result to string (for shell expansion)
char *arith_expand(t_shell *state, const char *expr, int len);
```

### Example Usage

```c
bool error;
long long result = arith_eval(shell, "2 + 3 * 4", 9, &error);
// result = 14, error = false

char *str_result = arith_expand(shell, "(5 + 3) * 2", 11);
// str_result = "16"
```

## Supported Features

### Operators (by precedence, highest to lowest)

1. **Unary operators**: `+`, `-`, `!`, `~`
2. **Exponentiation**: `**` (right-associative)
3. **Multiplicative**: `*`, `/`, `%`
4. **Additive**: `+`, `-`
5. **Shift**: `<<`, `>>`
6. **Relational**: `<`, `<=`, `>`, `>=`
7. **Equality**: `==`, `!=`
8. **Bitwise AND**: `&`
9. **Bitwise XOR**: `^`
10. **Bitwise OR**: `|`
11. **Logical AND**: `&&`
12. **Logical OR**: `||`
13. **Ternary**: `?:`
14. **Comma**: `,`

### Numeric Literals

- **Decimal**: `123`, `0`
- **Octal**: `0123` (leading zero)
- **Hexadecimal**: `0x123`, `0XAB`

### Variables

- Shell variables are automatically expanded: `$VAR` or `VAR`
- Undefined variables evaluate to 0

## Algorithm Details

### Lexical Analysis (Lexer)

The lexer uses a single-pass algorithm that:

1. **Skips whitespace** automatically
2. **Recognizes number patterns** with base detection:
   ```c
   // Base detection logic
   if (input[pos] == '0' && pos + 1 < len) {
       if (input[pos + 1] == 'x' || input[pos + 1] == 'X')
           return 16; // Hexadecimal
       else if (isdigit(input[pos + 1]))
           return 8;  // Octal
   }
   return 10; // Decimal
   ```
3. **Handles multi-character operators** like `**`, `<<`, `>=`, etc.
4. **Identifies variables** using C identifier rules

### Syntax Analysis (Parser)

The parser implements **recursive descent parsing** with separate functions for each precedence level:

```
Expression Grammar (simplified):
expr        ::= ternary (',' ternary)*
ternary     ::= or ('?' expr ':' ternary)?
or          ::= and ('||' and)*
and         ::= bitor ('&&' bitor)*
bitor       ::= bitxor ('|' bitxor)*
bitxor      ::= bitand ('^' bitand)*
bitand      ::= equality ('&' equality)*
equality    ::= relational (('==' | '!=') relational)*
relational  ::= shift (('<' | '<=' | '>' | '>=') shift)*
shift       ::= additive (('<<' | '>>') additive)*
additive    ::= multiplicative (('+' | '-') multiplicative)*
multiplicative ::= exponent (('*' | '/' | '%') exponent)*
exponent    ::= unary ('**' exponent)?  // Right-associative
unary       ::= ('+' | '-' | '!' | '~') unary | primary
primary     ::= number | variable | '(' expr ')'
```

### Key Design Decisions

1. **Right Associativity for Exponentiation**:
   ```c
   // 2**3**2 = 2**(3**2) = 2**9 = 512
   long long arith_parse_exponent(t_arith_parser *p) {
       long long base = arith_parse_unary(p);
       if (peek_token() == ATOK_POW) {
           advance();
           return power(base, arith_parse_exponent(p)); // Recursive call
       }
       return base;
   }
   ```

2. **Error Handling**:
   - Division by zero detection
   - Syntax error propagation through the parser
   - Graceful recovery without crashes

3. **Variable Resolution**:
   ```c
   long long get_var_value(t_arith_parser *p, const char *name, int len) {
       char *key = ft_strndup(name, len);
       char *val = env_expand_n(p->shell, key, len);
       // Convert string to integer, default to 0 if invalid
       return val ? ft_checked_atoi(val, &result, 42) : 0;
   }
   ```

## Tricky Implementation Details

### 1. Token Look-ahead
The parser uses a single token look-ahead system:
```c
t_arith_token arith_lexer_peek(t_arith_lexer *lex) {
    return lex->current; // Current token cached after advance
}
```

### 2. Multi-character Operator Handling
```c
void lex_two_char_op(t_arith_lexer *lex, char c2, 
                     t_arith_tok single, t_arith_tok dbl) {
    if (lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == c2) {
        lex->current.type = dbl;    // Double-char operator
        lex->pos += 2;
    } else {
        lex->current.type = single; // Single-char operator
        lex->pos++;
    }
}
```

### 3. Precedence Climbing
Each precedence level has its own parsing function that calls the next higher precedence level:
```c
// Lower precedence calls higher precedence
long long arith_parse_additive(t_arith_parser *p) {
    long long left = arith_parse_multiplicative(p); // Higher precedence
    while (is_additive_op(peek())) {
        // Handle same precedence level
    }
}
```

### 4. Memory Management
- **No dynamic allocation during parsing** - uses stack for recursion
- **String results allocated once** in `arith_expand()`
- **Variable names use string slices** instead of copying

## Error Handling

The parser implements a **panic-mode error recovery** system:

1. **Lexical errors**: Invalid characters set error flag
2. **Syntax errors**: Missing expected tokens
3. **Semantic errors**: Division by zero, overflow protection
4. **Error propagation**: Errors bubble up through recursive calls

```c
void expect(t_arith_parser *p, t_arith_tok type) {
    if (p->lexer->current.type != type) {
        p->error = true; // Set error flag
        return;
    }
    arith_lexer_advance(p->lexer);
}
```

## Integration with Shell

The arithmetic module integrates with the shell through:

1. **Variable expansion**: Uses shell's environment system
2. **Error reporting**: Reports to shell's error ctx
3. **Memory management**: Compatible with shell's allocation patterns

## Performance Characteristics

- **Time Complexity**: O(n) where n is expression length
- **Space Complexity**: O(d) where d is expression depth (recursion stack)
- **Memory**: Minimal allocations, mostly stack-based
- **Typical Use**: Fast evaluation of shell arithmetic expressions

## Testing Considerations

Key test cases should cover:
- **Operator precedence**: `2 + 3 * 4` should equal `14`
- **Associativity**: `2**3**2` should equal `512`
- **Edge cases**: Division by zero, integer overflow
- **Variable expansion**: Undefined variables, complex expressions
- **Number bases**: Octal, hex, decimal parsing
- **Error recovery**: Malformed expressions

This arithmetic parser provides a robust foundation for shell arithmetic operations while maintaining clean separation of concerns and predictable behavior.