# create executable
add_executable(cachemgr
    basic_utils_logger.hpp
    cli_opts.hpp
    main.cpp
)

SetupTarget(cachemgr "cachemgr")

# include dependencies
target_link_libraries(cachemgr PRIVATE cachemgr-utils)
target_link_libraries(cachemgr PRIVATE libcachemgr)
target_link_libraries(cachemgr PRIVATE quill::quill)
target_link_libraries(cachemgr PRIVATE fmt)
target_link_libraries(cachemgr PRIVATE libs::argparse)
