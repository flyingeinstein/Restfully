project(basic-tests)

# SOURCE_FILES requests.h RestEndpointsTests.cpp RestRequestTests.cpp RestRequestVptrTests.cpp)
set(SOURCE_FILES basic-tests.cpp StringPoolTests.cpp Arguments.cc pagedpool.cc HandlerTests.cpp)

add_executable(basic-tests ${SOURCE_FILES})
add_dependencies(basic-tests restfully)

#set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(TARGET basic-tests PROPERTY CXX_STANDARD 11)
#target_compile_features(basic-tests PUBLIC cxx_generalized_initializers)

include_directories(../../src ../catch2)
target_link_libraries(basic-tests restfully)

if(VALGRIND)
    add_dependencies(memcheck memcheck-basic-tests)
    add_custom_target(memcheck-basic-tests
            COMMAND "${VALGRIND}" --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes --free-fill=0xde --malloc-fill=0xba --redzone-size=64 $<TARGET_FILE:basic-tests>)
endif()

install(TARGETS basic-tests
        RUNTIME DESTINATION "${APP_INSTALL_PREFIX}/tests")

# include the generated list of cmake tests
include(CMakeTests.txt)
