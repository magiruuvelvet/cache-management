# Cache Management

A work-in-progress utility to tame package managers and keep your caches under control.
Because your storage is not free real estate.

*I highly recommend reading the [Concept Document](./CONCEPT.md) on why this utility exists.*

Example output ([*commit*](https://github.com/magiruuvelvet/cache-management/commit/ec156a06cb2ee1a4ff5a8d221e010154e072bb82))

```
Calculating usage statistics...
/home/magiruuvelvet/.gradle         -> /caches/1000/gradle     :   4.79GB (4791587980 bytes)
/caches/1000/node_modules                                      :   1.74GB (1742599173 bytes)
/home/magiruuvelvet/.npm            -> /caches/1000/npm        : 299.15MB (299146736 bytes)
/home/magiruuvelvet/.pub-cache      -> /caches/1000/pub-cache  : 282.39MB (282389093 bytes)
/home/magiruuvelvet/.dartServer     -> /caches/1000/dartServer : 216.64MB (216636701 bytes)
/tmp/preamble-*.pch                                            : 210.98MB (210975016 bytes)
/home/magiruuvelvet/.cargo          -> /caches/1000/cargo      : 191.27MB (191267914 bytes)
/home/magiruuvelvet/.m2             -> /caches/1000/m2         : 106.89MB (106892892 bytes)
/caches/1000/cmake                                             :  23.10MB (23098756 bytes)
/home/magiruuvelvet/.node-gyp       -> /caches/1000/node-gyp   :  12.23MB (12230361 bytes)
/home/magiruuvelvet/.cache/composer -> /caches/1000/composer   :      13B (13 bytes)
/home/magiruuvelvet/.bundle         -> /caches/1000/bundle     :       0B (0 bytes)
/home/magiruuvelvet/.cache/clangd   -> /caches/1000/clangd     :       0B (0 bytes)
/home/magiruuvelvet/.dub            -> /caches/1000/dub        :       0B (0 bytes)
/home/magiruuvelvet/.go             -> /caches/1000/go         :       0B (0 bytes)
/home/magiruuvelvet/.cache/go-build -> /caches/1000/go-build   :       0B (0 bytes)
/home/magiruuvelvet/.cache/zig      -> /caches/1000/zig        :       0B (0 bytes)
/home/magiruuvelvet/.cache/zls      -> /caches/1000/zls        :       0B (0 bytes)
                                                    total size :   7.88GB (7876824635 bytes)
                                 available space on cache root :  28.28GB (28276158464 bytes)
```

## Requirements

- C++20 compiler
- C++ standard library with `<filesystem>` support
- CMake 3.25 or higher

### Bundled dependencies

This project uses git submodules to manage its bundled dependencies.
Ensure to make a recursive clone with `git clone --recursive`.
If you have already cloned the repository without submodules, you can run `git submodule update --init --recursive` instead.

 - **[rapidyaml](https://github.com/biojppm/rapidyaml)**: YAML is used to configure this application.
 - **[{fmt}](https://github.com/fmtlib/fmt)**: Modern formatting library.
 - **[quill](https://github.com/odygrd/quill)**: used for logging.
 - **[simdjson](https://github.com/simdjson/simdjson)**: This library was chosen for the JSON parsing support in the package manager module. It works without exceptions and is also super fast.
 - **[argparse-cpp](https://github.com/magiruuvelvet/argparse-cpp)**: Simple stupid command line parser for C++
 - **[sqlite3](https://www.sqlite.org)**\*: A SQLite database is used for local cache analytics. SQLite was chosen for flexibility, portability and ease-of-use. This allows you to easily analyze the database using 3rd party applications.

\* *can be build and linked against the system shared version if desired*

### Testing dependencies

 - **[Catch2](https://github.com/catchorg/Catch2)**: Testing framework

## Supported Operating Systems

As of right now, this utility only works on Linux, but it might compile and run on BSD platforms too.

To add support for other operating system, you technically only need to extend [`src/utils`](./src/utils).
If changes to the remaining codebase are needed, then this is considered a bug in the architecture of the application.

## Building

Out of source builds are recommended.

```sh
cmake -B ./build
cmake --build ./build
```

## Configuration

The configuration is stored in `$XDG_CONFIG_HOME/cachemgr/cachemgr.yaml`
(`~/.config/cachemgr/cachemgr.yaml` if `XDG_CONFIG_HOME` is not set).
The log file is stored in `$XDG_CACHE_HOME/cachemgr/cachemgr.log`
(`~/.cache/cachemgr/cachemgr.log` if `XDG_CACHE_HOME` is not set).

For an example configuration, see [test.yaml](./test/assets/test.yaml) from the unit tests.
