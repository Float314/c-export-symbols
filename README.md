# C-Export-Symbol

A simple command-line tool that automatically generates C export headers from your C source files using **libclang**.

## Features

* Generate C export headers automatically
* Uses **libclang** for accurate parsing
* Simple JSON-based configuration
* Minimal command-line arguments

## Usage

Instead of passing dozens of command-line flags, configure the project using a JSON file.

**Example configuration**

```json
{
    "files": [
        "module1.c",
        "module67.c"
    ],

    "exports-symbol-directory": "incl/"
}
```

> **Note:** It is recommended to add this configuration file to your `.gitignore` if it contains project-specific paths.

Generate the export headers using:

```bash
c-exports-symbol export config.json
```

Eg, file module1.c
```c
#include <cstdio.h>

int add(int a, int b) {
    return a + b;
}

```

Therefore, the output module - \
```c
#include <cstdio.h>
int add(int a, int b);
```

## Dependencies

* libclang
* matjson

## License

This project is licensed under the BSD 4-clause License. 
