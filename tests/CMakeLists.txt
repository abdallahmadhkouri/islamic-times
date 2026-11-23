find_package(Qt6 REQUIRED COMPONENTS Test)

set(TEST_SOURCES
    tst_types.cpp
    tst_hijriyear.cpp
    tst_eventcalendar.cpp
    tst_statemachine.cpp
    tst_iqamahparser.cpp
    tst_worldwide.cpp
    tst_nosource.cpp
)

foreach(source ${TEST_SOURCES})
    get_filename_component(name ${source} NAME_WE)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE islamic-times Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endforeach()
