cmake_minimum_required(VERSION 3.15)
project(pqc_memory_benchmark C)

set(CMAKE_C_STANDARD 11)

# -O0 desliga otimizações para preservar frames de stack reais.
# Sem isso, o compilador pode inline funções e reduzir artificialmente
# o stack usage medido.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -Wall")

# ----------------------------------------------------------------
# Procura liboqs instalada via cmake --install
# Passe -DCMAKE_PREFIX_PATH=/caminho/para/liboqs_install ao cmake
# ----------------------------------------------------------------
find_package(liboqs QUIET)

if(liboqs_FOUND)
    message(STATUS "liboqs encontrada via find_package")
    set(OQS_TARGET OQS::oqs)
else()
    # Fallback: busca manualmente pelo header e biblioteca
    message(STATUS "find_package não encontrou liboqs — tentando busca manual")

    find_path(OQS_INCLUDE_DIR
        NAMES oqs/oqs.h
        HINTS
            ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/liboqs_install/include
            /usr/local/include
            /usr/include
    )
    find_library(OQS_LIBRARY
        NAMES oqs
        HINTS
            ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/liboqs_install/lib
            ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/liboqs_install/lib64
            /usr/local/lib
            /usr/lib
    )

    if(NOT OQS_INCLUDE_DIR OR NOT OQS_LIBRARY)
        message(FATAL_ERROR
            "liboqs não encontrada.\n"
            "Compile e instale primeiro:\n"
            "  cd ../third_party/liboqs && mkdir build && cd build\n"
            "  cmake .. -DCMAKE_INSTALL_PREFIX=../../liboqs_install "
            "-DOQS_BUILD_ONLY_LIB=ON\n"
            "  make -j$(nproc) && make install\n"
            "Depois rode cmake com:\n"
            "  cmake .. -DCMAKE_PREFIX_PATH=../third_party/liboqs_install\n"
        )
    endif()

    add_library(oqs_manual INTERFACE)
    target_include_directories(oqs_manual INTERFACE ${OQS_INCLUDE_DIR})
    target_link_libraries(oqs_manual INTERFACE ${OQS_LIBRARY})
    set(OQS_TARGET oqs_manual)

    message(STATUS "liboqs header : ${OQS_INCLUDE_DIR}")
    message(STATUS "liboqs lib    : ${OQS_LIBRARY}")
endif()

# ----------------------------------------------------------------
# Executável principal — benchmark de stack/heap/RSS
# ----------------------------------------------------------------
add_executable(memory_benchmark memory_benchmark.c)
target_link_libraries(memory_benchmark PRIVATE ${OQS_TARGET} pthread)

# ----------------------------------------------------------------
# Runner individual para Valgrind massif
# Compila com -O0 -g para que o massif veja frames reais.
# ----------------------------------------------------------------
add_executable(massif_single massif_single.c)
target_link_libraries(massif_single PRIVATE ${OQS_TARGET})

message(STATUS "")
message(STATUS "Targets disponíveis:")
message(STATUS "  memory_benchmark  — benchmark stack/heap/RSS completo")
message(STATUS "  massif_single     — runner individual para valgrind massif")
message(STATUS "")
message(STATUS "Valgrind massif (via run_massif_all.sh):")
message(STATUS "  cd build && ../run_massif_all.sh")
message(STATUS "")
