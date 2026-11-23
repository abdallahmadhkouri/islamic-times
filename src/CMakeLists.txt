add_library(islamic-times STATIC
    core/Types.cpp
    core/HijriYear.cpp
    core/EventCalendar.cpp
    core/DayRules.cpp
    store/PayloadCache.cpp
    core/PrayerStateMachine.cpp
    providers/IqamahParser.cpp
    providers/IqamahClient.cpp
)
target_include_directories(islamic-times PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(islamic-times PUBLIC Qt6::Core)

find_package(Qt6 6.5 REQUIRED COMPONENTS Network)
add_library(islamic-times-net STATIC net/QNetworkTransport.cpp)
target_link_libraries(islamic-times-net PUBLIC islamic-times Qt6::Network)
