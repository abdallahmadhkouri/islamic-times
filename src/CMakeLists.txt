add_library(islamic-times STATIC
    core/Types.cpp
    core/HijriYear.cpp
    core/EventCalendar.cpp
    core/DayRules.cpp
    core/PrayerStateMachine.cpp
    providers/IqamahParser.cpp
)
target_include_directories(islamic-times PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(islamic-times PUBLIC Qt6::Core)
