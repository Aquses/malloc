cmake_minimum_required(VERSION 3.0)

project(YourProjectNameHere)

# Add your source files
set(SOURCE_FILES
    deque.c
    deque.h
    list.c
    list.h
    list_iterator.c
    list_node.c
    set.h
    set.c
    main.c

)

# Create an executable from the source files
add_executable(main ${SOURCE_FILES})
