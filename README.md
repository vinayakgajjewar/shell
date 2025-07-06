# shell

A bare-bones shell written in C

```
cmake --build cmake-build-debug --target shell -j 10
cmake-build-debug/shell
```

**Limitations:**

- No multi-line comments
- No piping/redirection
- No quoting arguments
- No escaping whitespace