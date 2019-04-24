# cpray
(pronounced "spray")

Using C?...Pray! 

# Why?

When you find old C code projects from decades ago, you don't really know how the code-paths work. In some cases, this predates `strace` and profiling. Especially within a singleton C program, your compiler may lose function-level metadata.

# What?

This program, when run from a directory, will iteratively edit every C/CPP/C++ file and:
1. If it doesn't find `#include <stdio.h>`, add the line at the very top of the file.
2. After every single code block open (`{`), inject: `printf("cpray,<filename>,<lineno>\n")`

This program is a hail-mary when you want to trace execution and control flow. The output is CSV-compliant, so easy to import and parse by various tools. Intentionally not JSON because the hypothetical decades-old-C-program-environment may not have JSON support, predating it by a comfortable margin.
