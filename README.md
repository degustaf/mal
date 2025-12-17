# MAL

MAL is a clojure inspired lisp originally implemented by [Joel Martin](https://github.com/kanaka/mal).
This version is a bytecode intepreter intended to be used as an embedded scripting language.

## Building

```bash
cmake -S . -B build
cmake --build build/
```

## Inspirations and Influences

Beyond the obvious influence of the original MAL implementations, there are a number
of other influences:

1. [Crafting Interpreters](https://www.craftinginterpreters.com/) by Robert Nystrom
provided the inspiration for making it a bytecode interpreter. Also a number of
implementation details are based on that bytecode interpreter.

2. [lua](https://github.com/lua/lua/) provided the inspiration for using a register
machine instead of stack machine. In particular, the engine for one pass compiling
expressions expressions.
