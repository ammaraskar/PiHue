PROJECT(PiHue)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

include_directories(..)

add_executable(pihue 
    main.cpp
    WebcamFetcher.h WebcamFetcher.cpp
    HueConnector.h HueConnector.cpp)

target_link_libraries(pihue ${OpenCV_LIBS})
target_link_libraries(pihue huestream)
