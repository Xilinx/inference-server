list(APPEND targets
    # predict_api
    # fake_predict_api
    manager
    worker_info_buffers_infinite
    worker_info_buffers_finite
    # data_types
    # interface
)

foreach(target ${targets})
    add_library(fake_${target} OBJECT ${target}.cpp)
    target_include_directories(fake_${target} BEFORE PRIVATE ${PROTEUS_TEST_INCLUDE_DIRS})
    target_include_directories(fake_${target} AFTER PRIVATE ${PROTEUS_INCLUDE_DIRS})
    enable_ipo_on_target(fake_${target})

    list(APPEND CORE_OBJS $<TARGET_OBJECTS:fake_${target}>)
endforeach()

# target_link_libraries(predict_api INTERFACE jsoncpp)
# target_link_libraries(worker_info INTERFACE ${CMAKE_DL_LIBS})
# if(${PROTEUS_ENABLE_VITIS})
#   target_link_libraries(data_types INTERFACE xir)
# endif()

add_library(fake_core INTERFACE)
target_link_libraries(fake_core INTERFACE
  ${CORE_OBJS}
  ${targets}
)
