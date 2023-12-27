file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h)
add_executable(wmymain ${srcs})
target_include_directories(wmymain PUBLIC include)
target_link_libraries(wmymain PUBLIC webserver)