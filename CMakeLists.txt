cmake_minimum_required(VERSION 3.10)

project(winsock-chat)

if(WIN32)
    set(
        BASE_SRC 
        src/protocol/protocol.c
        src/server/server.c
        src/client/client.c
    )
    set(SERVER_SRC src/server/main.c ${BASE_SRC})
    set(CLIENT_SRC src/client/main.c ${BASE_SRC})

    include_directories(src)

    add_executable(server ${SERVER_SRC} src/server/server.c)
    add_executable(client ${CLIENT_SRC} src/client/client.c)

    target_link_libraries(server ws2_32)
    target_link_libraries(client ws2_32)

endif()