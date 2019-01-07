project(common-tests)

set(SOURCE_FILES tests.h tests.cc stress-tester.h)

add_library(common-tests STATIC ${SOURCE_FILES})

set_target_properties(common-tests PROPERTIES
        POSITION_INDEPENDENT_CODE 1
        C_STANDARD 99)
