file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h)
add_library(webserver STATIC ${srcs})
target_include_directories(webserver PUBLIC include)
target_link_libraries(webserver PUBLIC mysqlclient pthread)