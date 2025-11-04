# Regex Automata

This project implements an automaton to analyze and validate regular expressions. It uses a structure based on finite state machines to evaluate input strings according to a predefined regular expression.

## Features

- **Automata**: Base class defining the fundamental logic of a state machine.
- **RegexParser**: Implements a specific automaton for regular expression evaluation.
- **RegexState**: Represents different states within the evaluation process of a regular expression.
- **Support for quantifiers** (`*`, `+`, `?`, `{min,max}`).
- **Handling of groups and logical operators** (`|`, `()`).
- **Efficient validation optimized with appropriate data structures**.

## Usage

To use the regular expression parser:

```cpp
#include "RegexParser.hpp"

int main() {
    std::string regex = "a*b+c?";
    RegexParser parser(regex);
    
    if (parser.Match("aaabb")) {
        std::cout << "Valid string" << std::endl;
    } else {
        std::cout << "Invalid string" << std::endl;
    }
    return 0;
}
```