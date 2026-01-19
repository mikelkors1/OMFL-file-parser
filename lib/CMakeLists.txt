add_library(ITMLparse parser.cpp value.cpp)

target_include_directories(ITMLparse
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)