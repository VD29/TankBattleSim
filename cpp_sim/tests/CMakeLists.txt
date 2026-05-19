add_executable(tank_tests
    test_ecs.cpp
    test_threading.cpp
)

target_link_libraries(tank_tests
    PRIVATE
        tank_engine
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tank_tests)
