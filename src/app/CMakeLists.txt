# create executable
add_executable(cachemgr
    cachemgr.cpp
    cachemgr.hpp
    config.cpp
    config.hpp
    main.cpp
)

SetupTarget(cachemgr "cachemgr")

# include dependencies
target_link_libraries(cachemgr PRIVATE cachemgr-utils)
target_link_libraries(cachemgr PRIVATE ryml) # rapidyaml
