#include "lexer.h"

// Your scanner init is now just a local struct init.
t_scanner *init_scanner(const char *source)
{
    static t_scanner scanner;

    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
    return (&scanner);
}

// This is the main dispatch logic
t_token lexer_get_next_token(t_scanner *scan)
{
    const t_dispatch_data   *s;
    unsigned char           c;

    // 1. Get the singleton data (initializes on first call)
    s = get_dispatch_singleton();
    c = (unsigned char)*scan->current;

    // 2. FAST PATH (8-byte check)
    // If c is < 64 AND its bit is NOT set in the map, it's a literal.
    if (c < 64 && !(s->fast_path_map & (1ULL << c)))
    {
        return (handle_literal(scan));
    }

    // 3. FULL DISPATCH (128-entry table check)
    // If c is a special char (0-63) OR any char from 64-127.
    if (c < 128)
    {
        return (s->dispatch_table[c](scan));
    }

    // 4. Fallback for extended ASCII (128-255)
    return (handle_literal(scan));
}

/*
// --- Example main ---
int main(int argc, char **argv)
{
    t_scanner   scanner;
    
    if (argc == 2)
    {
        scanner = init_scanner(argv[1]);
        
        // Loop and get tokens...
        while (*scanner.current)
        {
            lexer_get_next_token(&scanner);
            // (Your token processing logic...)
            
            // This is just a dummy increment; your handlers 
            // (e.g., handle_literal) should advance the scanner.
            if (*scanner.current)
                scanner.current++; 
        }
    }
    return (0);
}
*/
