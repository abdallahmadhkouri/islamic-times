add_executable(it-inspector main.cpp)
target_link_libraries(it-inspector PRIVATE islamic-times Qt6::Quick)
target_compile_definitions(it-inspector PRIVATE
    ISLAMIC_TIMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")