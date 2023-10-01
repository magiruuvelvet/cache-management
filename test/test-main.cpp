#include <catch2/catch_all.hpp>
#include <catch2/internal/catch_leak_detector.hpp>
#include <catch2/catch_test_macros.hpp>

/** COPIED FROM `Catch2/src/catch2/internal/catch_main.cpp` **/

namespace Catch {
    CATCH_INTERNAL_START_WARNINGS_SUPPRESSION
    CATCH_INTERNAL_SUPPRESS_GLOBALS_WARNINGS
    static LeakDetector leakDetector;
    CATCH_INTERNAL_STOP_WARNINGS_SUPPRESSION
}

/**
 * @see Catch2/src/catch2/internal/catch_main.cpp
 */
inline int catch2_main(int argc, char **argv)
{
    // We want to force the linker not to discard the global variable
    // and its constructor, as it (optionally) registers leak detector
    (void)&Catch::leakDetector;

    return Catch::Session().run(argc, argv);
}

/** END OF COPIED CODE **/

/**
 * The real test program entry point.
 */
int main(int argc, char **argv)
{
    // nothing for now, just run catch2_main()
    return catch2_main(argc, argv);
}
