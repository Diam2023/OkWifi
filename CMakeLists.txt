set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)

file(GLOB USER_SRCS ./main/*.cc)

idf_component_register(SRCS ${USER_SRCS} INCLUDE_DIRS "./main")