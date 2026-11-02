cmake_minimum_required(VERSION 3.16)
project(pqc_benchmark C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ----------------------------------------------------------------
# Localiza liboqs
# ----------------------------------------------------------------
if(WIN32)
    set(OQS_DEFAULT_DIR "C:/liboqs")
else()
    set(OQS_DEFAULT_DIR "/usr/local")
endif()

set(OQS_DIR "${OQS_DEFAULT_DIR}" CACHE PATH "Raiz da instalacao da liboqs")

find_path(OQS_INCLUDE_DIR oqs/oqs.h PATHS "${OQS_DIR}/include" NO_DEFAULT_PATH)
find_library(OQS_LIBRARY oqs        PATHS "${OQS_DIR}/lib" "${OQS_DIR}/bin" NO_DEFAULT_PATH)

if(NOT OQS_INCLUDE_DIR OR NOT OQS_LIBRARY)
    message(FATAL_ERROR
        "\n[ERRO] liboqs nao encontrada em: ${OQS_DIR}\n"
        "Use: cmake -DOQS_DIR=<caminho> ..\n")
endif()

# ----------------------------------------------------------------
# Includes e fontes comuns a todos os executaveis
# ----------------------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR}/include ${OQS_INCLUDE_DIR})

set(COMMON_SOURCES
    src/core/timer.c
    src/core/stats.c
    src/core/utils.c
    src/kem/kem_benchmark.c
    src/kem/kyber_benchmark.c
    src/kem/hqc_benchmark.c
    src/sig/sig_benchmark.c
    src/sig/dilithium_benchmark.c
    src/sig/falcon_benchmark.c
    src/sig/sphincs_benchmark.c
    src/main.c
)

# ----------------------------------------------------------------
# Bibliotecas de linkagem
# ----------------------------------------------------------------
if(WIN32)
    set(LINK_LIBS ${OQS_LIBRARY} ssl crypto m)
else()
    set(LINK_LIBS ${OQS_LIBRARY} crypto m pthread)
endif()

# ----------------------------------------------------------------
# Funcao auxiliar: cria um executavel com flags especificas
#
# Uso:
#   add_bench(SUFFIX FLAGS)
#
# Exemplo:
#   add_bench("O2_native" "-O2;-march=native")
#
# Cria o executavel: full_bench-O2_native
# ----------------------------------------------------------------
function(add_bench SUFFIX FLAGS)
    set(TARGET_NAME "full_bench-${SUFFIX}")

    add_executable(${TARGET_NAME} ${COMMON_SOURCES})

    target_compile_options(${TARGET_NAME} PRIVATE
        -Wall -Wextra -Wno-unused-parameter
        ${FLAGS}
    )

    # -flto precisa estar na linkagem tambem
    if("${FLAGS}" MATCHES "flto")
        set_target_properties(${TARGET_NAME} PROPERTIES
            LINK_FLAGS "-flto"
        )
    endif()

    target_link_libraries(${TARGET_NAME} ${LINK_LIBS})

    # Cria a pasta results/ apos o build
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_BINARY_DIR}/results
        COMMENT "Criando diretorio results/"
    )

    message(STATUS "Registrado: ${TARGET_NAME}  flags: ${FLAGS}")
endfunction()

# ----------------------------------------------------------------
# Gera os 16 executaveis:
# 4 niveis de otimizacao x 4 combinacoes de flags
#
#  Ox             -> somente o nivel de otimizacao
#  Ox_native      -> nivel + march=native
#  Ox_lto         -> nivel + flto
#  Ox_native_lto  -> nivel + march=native + flto
# ----------------------------------------------------------------
foreach(OPT IN ITEMS O0 O1 O2 O3)
    add_bench("${OPT}"            "-${OPT}")
    add_bench("${OPT}_native"     "-${OPT};-march=native")
    add_bench("${OPT}_lto"        "-${OPT};-flto")
    add_bench("${OPT}_native_lto" "-${OPT};-march=native;-flto")
endforeach()

# ----------------------------------------------------------------
# Targets de execucao rapida para o nivel padrao (O2)
# Equivalentes a rodar ./full_bench-O2 [modo]
# ----------------------------------------------------------------
add_custom_target(run
    COMMAND ./full_bench-O2
    DEPENDS full_bench-O2
    COMMENT "Executando benchmark completo (O2)..."
)
add_custom_target(run_kem
    COMMAND ./full_bench-O2 kem
    DEPENDS full_bench-O2
)
add_custom_target(run_sig
    COMMAND ./full_bench-O2 sig
    DEPENDS full_bench-O2
)
add_custom_target(run_scale
    COMMAND ./full_bench-O2 scale
    DEPENDS full_bench-O2
)

# ----------------------------------------------------------------
# Resumo
# ----------------------------------------------------------------
message(STATUS "")
message(STATUS "  OQS include : ${OQS_INCLUDE_DIR}")
message(STATUS "  OQS library : ${OQS_LIBRARY}")
message(STATUS "  Plataforma  : ${CMAKE_SYSTEM_NAME}")
message(STATUS "")
message(STATUS "  Executaveis gerados (16 no total):")
message(STATUS "    full_bench-O0 / O0_native / O0_lto / O0_native_lto")
message(STATUS "    full_bench-O1 / O1_native / O1_lto / O1_native_lto")
message(STATUS "    full_bench-O2 / O2_native / O2_lto / O2_native_lto")
message(STATUS "    full_bench-O3 / O3_native / O3_lto / O3_native_lto")
message(STATUS "")
message(STATUS "  Uso:")
message(STATUS "    ./full_bench-O2                -> benchmark completo")
message(STATUS "    ./full_bench-O3_native scale   -> benchmark + escalabilidade")
message(STATUS "    cmake --build . --target run   -> atalho para O2 completo")
message(STATUS "")