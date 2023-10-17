cmake_minimum_required(VERSION 3.10)

project(winsock-chat)

if(WIN32)
    set(
        BASE_SRC 
        src/protocol/protocol.c
        src/server/server.cpp
        src/client/client.cpp
    )
    set(SERVER_SRC src/server/main.cpp ${BASE_SRC})
    set(CLIENT_SRC src/client/main.cpp ${BASE_SRC})

    set(CMAKE_CXX_STANDARD 20)

    include_directories(src)

    add_executable(server ${SERVER_SRC})
    add_executable(client ${CLIENT_SRC})

    target_link_libraries(server ws2_32)
    target_link_libraries(client ws2_32)

endif()