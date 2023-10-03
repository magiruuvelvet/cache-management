# create executable
add_executable(cachemgr
    main.cpp
)

SetupTarget(cachemgr "cachemgr")

# include dependencies
target_link_libraries(cachemgr PRIVATE cachemgr-utils)
target_link_libraries(cachemgr PRIVATE libcachemgr)
target_link_libraries(cachemgr PRIVATE quill::quill)
target_link_libraries(cachemgr PRIVATE fmt)
target_link_libraries(cachemgr PRIVATE libs::argparse)
