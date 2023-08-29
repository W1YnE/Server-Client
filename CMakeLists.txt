cmake_minimum_required(VERSION 3.10)

project(ProtoSocketApp)

# Find required protobuf package
find_package(Protobuf REQUIRED)

# Your source files here
set(CLIENT_SOURCES client.cpp)
set(SERVER_SOURCES server.cpp)

# Include directories for the project
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${PROTOBUF_INCLUDE_DIRS})

# Generate the .h and .cpp files from the .proto files
protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS account.proto)

# Create executable for client and server
add_executable(client ${CLIENT_SOURCES} ${PROTO_SOURCES} ${PROTO_HEADERS})
add_executable(server ${SERVER_SOURCES} ${PROTO_SOURCES} ${PROTO_HEADERS})

# Link the protobuf libraries
target_link_libraries(client ${PROTOBUF_LIBRARIES})
target_link_libraries(server ${PROTOBUF_LIBRARIES})

# Add any additional libraries or flags if needed
# For example, pthread library for threading support
# target_link_libraries(client pthread)
# target_link_libraries(server pthread)
