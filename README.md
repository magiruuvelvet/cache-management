# Cache Management

> **Note:**\
> See the [Concept Document](./CONCEPT.md) for details.

A work-in-progress utility to automate cache management.
Because your storage is not free real estate.

*I highly recommend reading the [Concept Document](./CONCEPT.md) on why this utility exists.*

## Requirements

- C++20 compiler
- C++ standard library with `<filesystem>` support
- CMake 3.25 or higher

## Supported Operating Systems

As of right now, this utility only works on Linux, but it might compile and run on BSD platforms too.

## Building

Out of source builds are recommended.

```sh
cmake -B ./build
cmake --build ./build
```
