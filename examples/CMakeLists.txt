include_directories (${sprig_SOURCE_DIR})

add_executable(scale-demo scale/main.c)
target_link_libraries (scale-demo sprig)
