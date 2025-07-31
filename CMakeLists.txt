cmake_minimum_required(VERSION 3.28)

project(SGBD)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules" ${CMAKE_MODULE_PATH})

# find_program(CLANGXX clang++)
# if(CLANGXX)
#   message(STATUS "clang++ encontrado: usando Clang en lugar de GCC")
# endif()
#set(CMAKE_CXX_COMPILER )

set(SOURCE_DIR src)
set(INCLUDE_DIR include)

set(MEGATRON_SOURCES
    ${SOURCE_DIR}/megatron/megatron.cpp
    ${SOURCE_DIR}/megatron/fill_meta.cpp
    ${SOURCE_DIR}/megatron/sql_ops/sql_ops.cpp
    ${SOURCE_DIR}/megatron/sql_ops/bulk_ops.cpp
    ${SOURCE_DIR}/megatron/sql_ops/sql_inserts.cpp
    ${SOURCE_DIR}/megatron/sql_ops/sql_selects.cpp
    ${SOURCE_DIR}/megatron/sql_ops/sql_selects_nth.cpp
    ${SOURCE_DIR}/megatron/sql_ops/sql_deletes.cpp
    ${SOURCE_DIR}/megatron/sql_ops/sql_deletes_nth.cpp
    ${SOURCE_DIR}/megatron/sql_ops/sql_updates.cpp
    ${SOURCE_DIR}/megatron/sql_ops/sql_updates_nth.cpp
    ${SOURCE_DIR}/megatron/pages/traverse_pages.cpp
    ${SOURCE_DIR}/megatron/pages/pages.cpp
    ${SOURCE_DIR}/megatron/hashes/add.cpp
    ${SOURCE_DIR}/megatron/hashes/get.cpp
    ${SOURCE_DIR}/megatron/hashes/hasher.cpp
    ${SOURCE_DIR}/megatron/hashes/hlpr.cpp
    ${SOURCE_DIR}/megatron/hashes/insert.cpp
    ${SOURCE_DIR}/megatron/hashes/remove.cpp
    ${SOURCE_DIR}/megatron/hashes/search.cpp
    ${SOURCE_DIR}/megatron/hashes/split.cpp
    ${SOURCE_DIR}/megatron/hashes/update.cpp
    ${SOURCE_DIR}/megatron/hashes/set_based.cpp
    ${SOURCE_DIR}/megatron/bptree/add.cpp
    ${SOURCE_DIR}/megatron/bptree/bptree.cpp
    ${SOURCE_DIR}/megatron/bptree/get.cpp
    ${SOURCE_DIR}/megatron/bptree/insert.cpp
    ${SOURCE_DIR}/megatron/bptree/remove.cpp
    ${SOURCE_DIR}/megatron/bptree/update.cpp
    ${SOURCE_DIR}/megatron/bptree/misc.cpp
    ${SOURCE_DIR}/megatron/bptree/search.cpp
    ${SOURCE_DIR}/megatron/bptree/split.cpp
    ${SOURCE_DIR}/megatron/bptree/set_based.cpp
    ${SOURCE_DIR}/megatron/ui/buffer_interact.cpp
    ${SOURCE_DIR}/megatron/ui/comparisons.cpp
    ${SOURCE_DIR}/megatron/ui/deletes.cpp
    ${SOURCE_DIR}/megatron/ui/disk.cpp
    ${SOURCE_DIR}/megatron/ui/inserts.cpp
    ${SOURCE_DIR}/megatron/ui/selects.cpp
    ${SOURCE_DIR}/megatron/ui/tables.cpp
    ${SOURCE_DIR}/megatron/ui/pages.cpp
    ${SOURCE_DIR}/megatron/ui/updates.cpp
    ${SOURCE_DIR}/megatron/ui/board.cpp
    ${SOURCE_DIR}/megatron/ui/utils.hpp
    ${SOURCE_DIR}/megatron/ui.cpp
    ${SOURCE_DIR}/megatron/registers.cpp
    ${SOURCE_DIR}/megatron/translate/translate.cpp
    ${SOURCE_DIR}/megatron/translate/reserved_sectors.cpp
    ${SOURCE_DIR}/megatron/translate/translate_data.cpp
    ${SOURCE_DIR}/megatron/translate/translate_hash.cpp
    ${SOURCE_DIR}/megatron/translate/translate_index.cpp
    ${SOURCE_DIR}/megatron/misc.cpp
    ${SOURCE_DIR}/board/generic.cpp
    ${SOURCE_DIR}/board/basic_orders.cpp
    ${SOURCE_DIR}/board/misc.cpp
)

set(DISK_MANAGER_SOURCES
    ${SOURCE_DIR}/disk_manager/disk_manager.cpp
    ${SOURCE_DIR}/disk_manager/block_reads.cpp
    ${SOURCE_DIR}/disk_manager/block_writes.cpp
    ${SOURCE_DIR}/disk_manager/header_ops.cpp
    ${SOURCE_DIR}/disk_manager/sector_reads.cpp
    ${SOURCE_DIR}/disk_manager/sector_writes.cpp
    ${SOURCE_DIR}/disk_manager/utils.cpp
)

set(BUFFER_MANAGER_SOURCES
    ${SOURCE_DIR}/buffer_manager/buffer_manager.cpp
    ${SOURCE_DIR}/buffer_manager/prints.cpp
    ${SOURCE_DIR}/buffer_manager/loads.cpp
    ${SOURCE_DIR}/buffer_manager/evictions.cpp
)

find_package(Boost REQUIRED)

add_library(megatron)
target_sources(megatron PRIVATE ${MEGATRON_SOURCES})
target_include_directories(megatron PUBLIC ${INCLUDE_DIR})

add_library(disk_manager)
target_sources(disk_manager PRIVATE ${DISK_MANAGER_SOURCES})
target_include_directories(disk_manager PUBLIC ${INCLUDE_DIR})
target_include_directories(disk_manager PRIVATE ${Boost_INCLUDE_DIRS})

add_library(buffer_manager)
target_sources(buffer_manager PRIVATE ${BUFFER_MANAGER_SOURCES})
target_include_directories(buffer_manager PUBLIC ${INCLUDE_DIR})
target_link_libraries(buffer_manager PRIVATE disk_manager)

add_executable(runner ${SOURCE_DIR}/main.cpp)
target_link_libraries(runner PRIVATE megatron disk_manager buffer_manager)

add_executable(tests ${SOURCE_DIR}/test.cpp)
target_link_libraries(tests PRIVATE megatron disk_manager buffer_manager)

add_executable(hash_test ${SOURCE_DIR}/hash_test.cpp)
target_link_libraries(hash_test PRIVATE megatron disk_manager buffer_manager)

add_executable(forito ${SOURCE_DIR}/forito.cpp)
target_link_libraries(forito PRIVATE megatron disk_manager buffer_manager)


add_custom_command(
    TARGET runner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:runner>/csv
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/csv
        $<TARGET_FILE_DIR:runner>/csv
)

add_custom_command(
    TARGET runner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:runner>/media
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/media
        $<TARGET_FILE_DIR:runner>/media
)
