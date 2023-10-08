# Cache Management

> **Note:**\
> See the [Concept Document](./CONCEPT.md) for details.

A work-in-progress utility to automate cache management.
Because your storage is not free real estate.

*I highly recommend reading the [Concept Document](./CONCEPT.md) on why this utility exists.*

Example output ([*commit*](https://github.com/magiruuvelvet/cache-management/commit/3e5188a1b24e28aef8bf599fe8409b4e76926e5c))

```
Calculating usage statistics...
/home/magiruuvelvet/.gradle         -> /caches/1000/gradle       :    4.3GB (4256997698 bytes)
                                       /caches/1000/node_modules :    1.8GB (1742599173 bytes)
/home/magiruuvelvet/.npm            -> /caches/1000/npm          :    288MB (287969227 bytes)
/home/magiruuvelvet/.pub-cache      -> /caches/1000/pub-cache    :  247.8MB (247702518 bytes)
/home/magiruuvelvet/.dartServer     -> /caches/1000/dartServer   :  180.4MB (180354868 bytes)
/home/magiruuvelvet/.m2             -> /caches/1000/m2           :  106.9MB (106892892 bytes)
                                       /caches/1000/cmake        :     22MB (21942598 bytes)
/home/magiruuvelvet/.node-gyp       -> /caches/1000/node-gyp     :   12.3MB (12230361 bytes)
/home/magiruuvelvet/.bundle         -> /caches/1000/bundle       :       0B (0 bytes)
/home/magiruuvelvet/.cargo          -> /caches/1000/cargo        :       0B (0 bytes)
/home/magiruuvelvet/.cache/clangd   -> /caches/1000/clangd       :       0B (0 bytes)
/home/magiruuvelvet/.cache/composer -> /caches/1000/composer     :       0B (0 bytes)
/home/magiruuvelvet/.dub            -> /caches/1000/dub          :       0B (0 bytes)
/home/magiruuvelvet/.go             -> /caches/1000/go           :       0B (0 bytes)
/home/magiruuvelvet/.cache/go-build -> /caches/1000/go-build     :       0B (0 bytes)
/home/magiruuvelvet/.cache/zig      -> /caches/1000/zig          :       0B (0 bytes)
/home/magiruuvelvet/.cache/zls      -> /caches/1000/zls          :       0B (0 bytes)
                                                      total size :    6.9GB (6856689335 bytes)
```

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
