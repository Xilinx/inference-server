
add_library(fakeBatcher OBJECT batcher.cpp)
target_include_directories(fakeBatcher BEFORE PRIVATE ${PROTEUS_TEST_INCLUDE_DIRS})
    target_include_directories(fakeBatcher AFTER PRIVATE ${PROTEUS_INCLUDE_DIRS})
target_link_libraries(fakeBatcher INTERFACE fake_observation Threads::Threads)
enable_ipo_on_target(fakeBatcher)
