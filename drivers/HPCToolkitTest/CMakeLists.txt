project(HPCToolkitTest)

add_executable(HPCToolkitTest "HPCToolkitTest.cpp")
target_link_libraries(HPCToolkitTest Camellia)

add_executable(HPCToolkitTestSimple "HPCToolkitTestSimple.cpp")