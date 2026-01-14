# Lexer 
The prior logic of my lexer is to use either a 
- Streaming scanner, which is low-memory, single-pass and has a simpler flow
for parser that consumes tokens immediately, faster when combined with fast-path dispatch (constant time handler)
  When needed to implement large lookahead/backtracking, we need an explicit token buffer, if parser needs to rewind or reparse.

- Deque-based:
  - easy random access, arbitrary lookahead/bactraccking, collect full token stream for debugging or multi-pass parsing, simple to implement.
  - More memory management (reallocs) lifetime management of input buffer (tokens hold pointers), 
  potential overhead if we always tokenize entire input.

- Hybrid approach: we use a streaming scanner as the primary mechanism. and only push tokens into a small deque when the parser need lookahead or buffering (on demand buffering). This gives best trade offs.

## To do the choice
Our parser need lookahead:

want to keep tokens for later stages
- history
- reprinting
- multi-pass analysis
- interactive prompts
- heredoc handling



# Debug

When building a `lexer`, hanlding edge cases is one of those
sneaky things that looks simple... but bite us hard we we don't test
every combo..

to Organize simply and in order to help the future tester to indicate
what kind of sequence to test. I'll wrap up all the sequences in one block and lower in this lexer documentation. We'll see further details
on some of them.

<details>
	<summary>click here to see the different sequences tested</summary>
	<div>
		<pre>
"alpha beta gamma"
"alpha\tbeta gamma"
"alpha beta                "
"alpha beta  "
alpha beta gamma delta
\t    \n
\t alpha \n beta\t\t gamma
alpha\nbeta\ngamm
alpha\nbeta\ngamma
alpha\tbeta gamma
alpah     beta     gamma
		</pre>
	</div>
</details>

-----------------------------------------------------------
## Error undefined behavior token

### Whitespace Error

#### space behavior expected


#### ERROR \<pattern\>
----
```bash
$>tokenfkjlalskjfja jkklsdlfsaddks sdkj fdsk j kjd  fd
 TOKEN_IDENTIFIER            | line 1 | tokenfkjlalskjfja
 TOKEN_IDENTIFIER            | line 1 | jkklsdlfsaddks
 TOKEN_IDENTIFIER            | line 1 | sdkj
 TOKEN_IDENTIFIER            | line 1 | fdsk
 TOKEN_IDENTIFIER            | line 1 | j
 TOKEN_IDENTIFIER            | line 1 | kjd
 TOKEN_WHITESPACE            | line 1 |  
 TOKEN_IDENTIFIER            | line 1 | fd
 TOKEN_EOF                   | line 1 | 
```

-------------------------------------------------------------

```bash
$>     alpha  beta    gamma
 TOKEN_WHITESPACE            | line 1 |     
 TOKEN_IDENTIFIER            | line 1 | alpha
 TOKEN_WHITESPACE            | line 1 |  
 TOKEN_IDENTIFIER            | line 1 | beta
 TOKEN_WHITESPACE            | line 1 |    
 TOKEN_IDENTIFIER            | line 1 | gamma
 TOKEN_EOF                   | line 1 | 
```

```bash

```
```bash

```

```bash

```

```bash

```

```bash

```

```bash

```


```bash

```

```bash

```

```bash

```
```bash

```

```bash

```

```bash

```

```bash

```

```bash

```


```bash

```

```bash

```

```bash

```
```bash

```

```bash

```

```bash

```

```bash

```

```bash

```


```bash

```


```bash

```

```bash

```
```bash

```

```bash

```

```bash

```

```bash

```

```bash

```


```bash

```


```bash

```

```bash

```
```bash

```

```bash

```

```bash

```

```bash

```

```bash

```


```bash

```

<style>
details {
  border: 1px solid #aaa;
  border-radius: 6px;
  padding: 0.5em 1em 0.5em 1em;
  background: #f9f9f9;
  margin-bottom: 1em;
  transition: box-shadow 0.3s;
}
details[open] {
  box-shadow: 0 2px 8px rgba(0,0,0,0.08);
  background: #eef6ff;
}
summary {
  font-weight: bold;
  cursor: pointer;
  outline: none;
  padding: 0.2em 0;
  color: #2a5db0;
  transition: color 0.2s;
}
summary:hover {
  color: #174080;
}
details > div {
  margin-top: 0.5em;
  animation: fadeIn 0.4s;
}
@keyframes fadeIn {
  from { opacity: 0; transform: translateY(-8px);}
  to   { opacity: 1; transform: translateY(0);}
}
pre {
  background: #222;
  color: #f8f8f2;
  padding: 1em;
  border-radius: 6px;
  font-size: 1.05em;
  line-height: 1.5;
  overflow-x: auto;
  box-shadow: 0 2px 8px rgba(0,0,0,0.10);
  margin: 0.5em 0;
  font-family: 'Fira Mono', 'Consolas', 'Monaco', monospace;
}
</style>