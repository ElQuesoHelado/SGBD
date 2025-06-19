project(runner)
project(tests)

set(CMAKE_CXX_STANDARD 23)

find_package(Boost REQUIRED)

add_library(megatron src/megatron/megatron.cpp
    src/megatron/disk_related.cpp
    src/megatron/sql_ops.cpp
    src/megatron/bulk_ops.cpp
    src/megatron/fill_meta.cpp
    src/megatron/sql_inserts.cpp
    src/megatron/sql_selects.cpp
    src/megatron/sql_deletes.cpp
    src/megatron/ui.cpp
    src/megatron/traverse_pages.cpp
    src/megatron/pages.cpp
    src/megatron/registers.cpp
    src/megatron/translate.cpp
    src/megatron/misc.cpp
    src/buffer/trash.cpp)

add_library(disk_manager src/disk_manager/disk_manager.cpp
    src/disk_manager/block_reads.cpp
    src/disk_manager/block_writes.cpp
    src/disk_manager/folder_ops.cpp
    src/disk_manager/header_ops.cpp
    src/disk_manager/sector_reads.cpp
    src/disk_manager/sector_writes.cpp
    src/disk_manager/utils.cpp
)

target_include_directories(megatron PUBLIC include)

target_include_directories(disk_manager PUBLIC include)
target_include_directories(disk_manager PRIVATE ${Boost_INCLUDE_DIRS})

add_executable(runner src/main.cpp)
add_executable(tests src/test.cpp)

target_link_libraries(runner PUBLIC megatron)
target_link_libraries(runner PUBLIC disk_manager)
target_link_libraries(tests PUBLIC megatron)
target_link_libraries(tests PUBLIC disk_manager)

add_custom_command(
    TARGET runner
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/csv
    $<TARGET_FILE_DIR:runner>/csv
)
