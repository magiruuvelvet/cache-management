# Cache Management

> **Note:**\
> See the [Concept Document](./CONCEPT.md) for details.

A work-in-progress utility to automate cache management.
Because your storage is not free real estate.

*I highly recommend reading the [Concept Document](./CONCEPT.md) on why this utility exists.*

Example output ([*commit*](https://github.com/magiruuvelvet/cache-management/commit/19637d66f87a5464f3f173c8f8e61be5b437e424))

```
Calculating usage statistics...
/home/magiruuvelvet/.gradle         -> /caches/1000/gradle       :    4.3GB (4256997698 bytes)
                                       /caches/1000/node_modules :    1.8GB (1742599173 bytes)
/home/magiruuvelvet/.npm            -> /caches/1000/npm          :  293.7MB (293625259 bytes)
/home/magiruuvelvet/.pub-cache      -> /caches/1000/pub-cache    :  247.8MB (247702518 bytes)
/home/magiruuvelvet/.dartServer     -> /caches/1000/dartServer   :  180.4MB (180354868 bytes)
/home/magiruuvelvet/.m2             -> /caches/1000/m2           :  106.9MB (106892892 bytes)
                                       /caches/1000/cmake        :   23.1MB (23063311 bytes)
/home/magiruuvelvet/.node-gyp       -> /caches/1000/node-gyp     :   12.3MB (12230361 bytes)
/home/magiruuvelvet/.cache/composer -> /caches/1000/composer     :      13B (13 bytes)
/home/magiruuvelvet/.bundle         -> /caches/1000/bundle       :       0B (0 bytes)
/home/magiruuvelvet/.cargo          -> /caches/1000/cargo        :       0B (0 bytes)
/home/magiruuvelvet/.cache/clangd   -> /caches/1000/clangd       :       0B (0 bytes)
/home/magiruuvelvet/.dub            -> /caches/1000/dub          :       0B (0 bytes)
/home/magiruuvelvet/.go             -> /caches/1000/go           :       0B (0 bytes)
/home/magiruuvelvet/.cache/go-build -> /caches/1000/go-build     :       0B (0 bytes)
/home/magiruuvelvet/.cache/zig      -> /caches/1000/zig          :       0B (0 bytes)
/home/magiruuvelvet/.cache/zls      -> /caches/1000/zls          :       0B (0 bytes)
                                                      total size :    6.9GB (6863466093 bytes)
                                   available space on cache root :   28.9GB (28832542720 bytes)
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

## Configuration

*Warning: It is not adviced to invest too much time configuring this on your system yet.*
*The configuration is not stabalized yet and undergoes breaking changes.*
*The minor version number will be increased on incompatible changes to the configuration,*
*as long as the application major version number is still at `0`.*

Currently the configuration is stored in `$XDG_CONFIG_HOME/cachemgr.yaml`
(`~/.config/cachemgr.yaml` if `XDG_CONFIG_HOME` is not set).
The log file is stored in `$XDG_CACHE_HOME/cachemgr.log`
(`~/.cache/cachemgr.log` if `XDG_CACHE_HOME` is not set).
Both locations are subject to change before the stable release, as they
are too generic.

For an example configuration, see [test.yaml](./test/assets/test.yaml) from the unit tests.
