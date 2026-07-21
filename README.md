# PQC Benchmark — Análise Experimental de Algoritmos NIST PQC

Benchmark comparativo de desempenho dos algoritmos finalistas do processo de padronização do NIST para Criptografia Pós-Quântica, desenvolvido como parte do TCC de Ciência da Computação da PUC Minas.

> **Autor:** Saulo de Moura Zandona Freitas
> **Orientador:** Matheus Alcântara Souza
> **Instituição:** Pontifícia Universidade Católica de Minas Gerais (PUC Minas)
> **Ano:** 2025

---

## Algoritmos Avaliados

| Família | Padrão NIST | Variantes | Tipo |
|---|---|---|---|
| ML-KEM (Kyber) | FIPS 203 | 512 / 768 / 1024 | KEM |
| HQC | 4ª Rodada — selecionado (2025) | 128 / 192 / 256 | KEM |
| ML-DSA (Dilithium) | FIPS 204 | 44 / 65 / 87 | Assinatura |
| FN-DSA (Falcon) | FIPS 206 (rascunho) | 512 / 1024 | Assinatura |
| SLH-DSA (SPHINCS+) | FIPS 205 | SHAKE-128s/f, 192s/f, 256s/f | Assinatura |

---

## Métricas Coletadas

1. **Tempo de execução** — média, desvio-padrão, mínimo e mediana em µs (n=200, 10 warmup)
2. **Tamanho dos artefatos** — chave pública, chave secreta, ciphertext/assinatura em bytes
3. **Impacto de flags de compilação** — 16 configurações (`-O0` a `-O3` × `-march=native` × `-flto`)
4. **Escalabilidade** — tempo de Sign e Verify para mensagens de 1 B a 1 MiB (21 pontos, potências de 2)
5. **Consumo de memória** — uso de pilha via *stack painting* e pico de heap via Valgrind massif

---

## Estrutura do Repositório

```
TCC_PQC/
├── README.md
├── code/
│   ├── tcc_pqc.tex                      # Artigo completo em LaTeX
│   ├── refs.bib                          # Referências bibliográficas
│   ├── graficos/                         # Figuras e gráficos do artigo (PDF)
│   ├── resumo_colunas_csv.md            # Documentação das colunas dos CSVs de memória
│   ├── fix_paragraphs.py                # Utilitário: formata parágrafos do .tex
│   │
│   ├── CMakeLists.txt                   # Build dos 16 executáveis de benchmark de tempo
│   ├── include/
│   │   ├── general_include.h            # Tipos centrais: kem_result_t, sig_result_t
│   │   ├── core/
│   │   │   ├── timer.h                  # Medição de tempo (CLOCK_MONOTONIC)
│   │   │   ├── stats.h                  # Estatísticas descritivas
│   │   │   └── utils.h                  # Configurações globais, CSV, utilitários
│   │   ├── kem/
│   │   │   ├── kem_benchmark.h          # Interface genérica para KEMs
│   │   │   ├── kyber_benchmark.h        # ML-KEM
│   │   │   └── hqc_benchmark.h          # HQC
│   │   └── sig/
│   │       ├── sig_benchmark.h          # Interface genérica + escalabilidade
│   │       ├── dilithium_benchmark.h    # ML-DSA
│   │       ├── falcon_benchmark.h       # FN-DSA
│   │       └── sphincs_benchmark.h      # SLH-DSA
│   ├── src/
│   │   ├── main.c                       # Orquestrador: modos, CSV paths, execução
│   │   ├── core/
│   │   │   ├── timer.c
│   │   │   ├── stats.c
│   │   │   └── utils.c
│   │   ├── kem/
│   │   │   ├── kem_benchmark.c
│   │   │   ├── kyber_benchmark.c
│   │   │   └── hqc_benchmark.c
│   │   └── sig/
│   │       ├── sig_benchmark.c
│   │       ├── dilithium_benchmark.c
│   │       ├── falcon_benchmark.c
│   │       └── sphincs_benchmark.c
│   │
│   ├── libs/
│   │   ├── liboqs/                      # Código-fonte da liboqs (submódulo)
│   │   └── liboqs_install/              # Instalação compilada da liboqs
│   │
│   └── benchmark/                       # Benchmark de consumo de memória
│       ├── CMakeLists.txt               # Build dos programas de memória
│       ├── build_and_run.sh             # Script: compila tudo e executa
│       ├── memory_benchmark.c           # Stack painting + heap delta + RSS
│       ├── massif_single.c              # Executor unitário para Valgrind massif
│       ├── run_massif_all.sh            # Script: roda massif para todos os algoritmos
│       ├── parse_massif.py              # Parser: extrai pico de heap dos .massif
│       ├── memory_results_tcc.csv       # Resultados de stack por algoritmo/operação
│       ├── massif_results_heap_tcc.csv  # Resultados de pico de heap por algoritmo/operação
│       └── build/                       # Binários compilados e arquivos .massif gerados
│           ├── memory_benchmark
│           ├── massif_single
│           └── massif_out/              # Arquivos .massif individuais (um por alg×op)
```

---

## Dependências

| Ferramenta | Versão testada |
|---|---|
| GCC | 13.3.0 |
| CMake | 3.28.3 |
| Ninja | 1.11.1 |
| liboqs | 0.10.1 |
| OpenSSL (dev) | 3.0.x |
| Valgrind | 3.22.0 (somente para benchmark de memória) |
| Python 3 | 3.12 (somente para benchmark de memória) |

### Instalar dependências (Ubuntu / Linux Mint)

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build libssl-dev git valgrind python3
```

---

## Compilar a liboqs

Clone diretamente no diretório esperado pelo projeto (`code/libs/liboqs`) e instale em `code/libs/liboqs_install`, que é o prefixo usado pelo benchmark de memória.

```bash
git clone --depth 1 --branch 0.10.1 \\
    https://github.com/open-quantum-safe/liboqs.git \\
    ~/TCC_PQC/code/libs/liboqs
cd ~/TCC_PQC/code/libs/liboqs

cmake -B build -GNinja \\
    -DCMAKE_BUILD_TYPE=Release \\
    -DBUILD_SHARED_LIBS=ON \\
    -DCMAKE_INSTALL_PREFIX=~/TCC_PQC/code/libs/liboqs_install \\
    -DOQS_ENABLE_KEM_HQC=ON \\
    -DOQS_ENABLE_KEM_ML_KEM=ON \\
    -DOQS_ENABLE_SIG_ML_DSA=ON \\
    -DOQS_ENABLE_SIG_SLH_DSA=ON \\
    -DOQS_ENABLE_SIG_FALCON=ON \\
    -DOPENSSL_ROOT_DIR=/usr \\
    -DOPENSSL_CRYPTO_LIBRARY=/usr/lib/x86_64-linux-gnu/libcrypto.so \\
    -DOPENSSL_SSL_LIBRARY=/usr/lib/x86_64-linux-gnu/libssl.so

cmake --build build -j$(nproc)
cmake --install build
```

> O passo `cmake --install build` é obrigatório: sem ele o diretório `libs/liboqs_install/` não é criado e o benchmark de memória (que usa esse prefixo) não compila.

---

## Benchmark de Desempenho Temporal

### Compilar

```bash
cd ~/TCC_PQC/code
mkdir -p build && cd build
cmake .. -DOQS_DIR=~/TCC_PQC/code/libs/liboqs/build
cmake --build . -j$(nproc)
```

O CMake gera **16 executáveis** automaticamente, um por configuração de compilação:

```
full_bench-O0             full_bench-O0_native
full_bench-O0_lto         full_bench-O0_native_lto
full_bench-O1             full_bench-O1_native
full_bench-O1_lto         full_bench-O1_native_lto
full_bench-O2             full_bench-O2_native
full_bench-O2_lto         full_bench-O2_native_lto
full_bench-O3             full_bench-O3_native
full_bench-O3_lto         full_bench-O3_native_lto
```

| Sufixo | Flags de compilação |
|---|---|
| `Ox` | `-Ox` |
| `Ox_native` | `-Ox -march=native` |
| `Ox_lto` | `-Ox -flto` |
| `Ox_native_lto` | `-Ox -march=native -flto` |

### Modos de execução

```bash
./full_bench-O2 [modo]
```

| Modo | O que executa |
|---|---|
| `all` | Todos os algoritmos (sem escalabilidade) |
| `scale` | Todos os algoritmos + escalabilidade de mensagem |
| `kem` | Apenas ML-KEM e HQC |
| `sig` | Apenas ML-DSA, FN-DSA e SLH-DSA |
| `mlkem` | Apenas ML-KEM |
| `hqc` | Apenas HQC |
| `mldsa` | Apenas ML-DSA |
| `falcon` | Apenas FN-DSA |
| `sphincs` | Apenas SLH-DSA |
| `scale_mldsa` | ML-DSA + escalabilidade |
| `scale_falcon` | FN-DSA + escalabilidade |
| `scale_sphincs` | SLH-DSA + escalabilidade |

### Rodar todos os 16 executáveis

```bash
cd build
for exec in \\
    full_bench-O0 full_bench-O0_native full_bench-O0_lto full_bench-O0_native_lto \\
    full_bench-O1 full_bench-O1_native full_bench-O1_lto full_bench-O1_native_lto \\
    full_bench-O2 full_bench-O2_native full_bench-O2_lto full_bench-O2_native_lto \\
    full_bench-O3 full_bench-O3_native full_bench-O3_lto full_bench-O3_native_lto; do
    echo "====== $exec ======"
    ./$exec scale
done
```

### Atalhos CMake

```bash
cmake --build . --target run        # modo all
cmake --build . --target run_kem    # modo kem
cmake --build . --target run_sig    # modo sig
cmake --build . --target run_scale  # modo scale
```

### Resultados gerados

Os CSVs são salvos em `build/results/`, organizados por configuração:

```
build/results/
├── O0/
│   ├── kem.csv
│   ├── sig.csv
│   └── sig_scale.csv
├── O0_native/ ...
├── O2/ ...
└── O3_native_lto/
    ├── kem.csv
    ├── sig.csv
    └── sig_scale.csv
```

#### Colunas de `kem.csv`

```
algoritmo_nist, algoritmo_oqs, nivel_nist,
pk_bytes, sk_bytes, ct_bytes, ss_bytes,
keygen_mean_us, keygen_stddev_us, keygen_min_us, keygen_median_us,
encaps_mean_us, encaps_stddev_us, encaps_min_us, encaps_median_us,
decaps_mean_us, decaps_stddev_us, decaps_min_us, decaps_median_us
```

#### Colunas de `sig.csv`

```
algoritmo_nist, algoritmo_oqs, nivel_nist,
pk_bytes, sk_bytes, sig_max_bytes, sig_real_bytes,
keygen_mean_us, keygen_stddev_us, keygen_min_us, keygen_median_us,
sign_mean_us,   sign_stddev_us,   sign_min_us,   sign_median_us,
verify_mean_us, verify_stddev_us, verify_min_us, verify_median_us
```

#### Colunas de `sig_scale.csv`

```
algoritmo_nist, nivel_nist, msg_bytes,
sign_mean_us,  sign_stddev_us,  sign_min_us,  sign_median_us,
verify_mean_us, verify_stddev_us, verify_min_us, verify_median_us
```

---

## Benchmark de Consumo de Memória

### Opção A — Script automático (recomendado)

```bash
cd ~/TCC_PQC/code/benchmark
bash build_and_run.sh
```

Compila a liboqs (se necessário), compila os programas de memória e executa o benchmark de stack. O CSV é salvo em `benchmark/memory_results_tcc.csv`.

### Opção B — Passo a passo manual

#### Compilar

```bash
cd ~/TCC_PQC/code/benchmark
mkdir -p build && cd build
cmake .. -DCMAKE_PREFIX_PATH=../../libs/liboqs_install
cmake --build . --parallel $(nproc)
```

#### Executar benchmark de stack

```bash
cd ~/TCC_PQC/code/benchmark/build
./memory_benchmark
```

#### Executar Valgrind massif para todos os algoritmos

```bash
cd ~/TCC_PQC/code/benchmark/build
bash ../run_massif_all.sh
```

Salva os arquivos `.massif` em `build/massif_out/` e gera `massif_results_heap_tcc.csv`.

#### Executar `massif_single` manualmente

```bash
# Testa um algoritmo diretamente (sem Valgrind):
./massif_single ML-KEM-512 keygen
./massif_single FN-DSA-512 verify      # aceita nome NIST
./massif_single Falcon-512 verify      # aceita nome liboqs

# Com Valgrind massif:
valgrind --tool=massif \\
         --pages-as-heap=no \\
         --massif-out-file=massif.ML-DSA-44.sign.out \\
         ./massif_single ML-DSA-44 sign

# Visualizar resultado:
ms_print massif.ML-DSA-44.sign.out | head -40
```

### Resultados gerados

| Arquivo | Conteúdo |
|---|---|
| `benchmark/memory_results_tcc.csv` | Uso de stack (bytes) por algoritmo e operação |
| `benchmark/massif_results_heap_tcc.csv` | Pico de heap (KB/MB) por algoritmo e operação |

Para descrição detalhada de cada coluna, consulte [`code/resumo_colunas_csv.md`](code/resumo_colunas_csv.md).

---

## Detalhes de Implementação

### Timer

A medição de tempo usa `clock_gettime(CLOCK_MONOTONIC)` no Linux. As funções são implementadas como `static inline` para eliminar overhead de chamada nas medições críticas.

### Verificação de corretude

A cada iteração do benchmark o resultado é verificado:

- **KEMs**: os shared secrets gerados por Encaps e Decaps são comparados byte a byte.
- **Assinaturas**: o retorno de Verify é verificado como `OQS_SUCCESS`.

Falhas de corretude abortam o benchmark com mensagem de erro.

### Detecção automática de configuração

Cada executável detecta seu próprio nível de otimização a partir do nome (`argv[0]`): `full_bench-O2_native` extrai o sufixo `O2_native` e o usa como subdiretório de resultados e rótulo no banner.

### Stack painting (medição de pilha)

Um buffer de 4 MB é preenchido com o byte sentinela `0xAB` e passado como pilha de uma thread POSIX via `pthread_attr_setstack()`. Após a execução da operação criptográfica, o buffer é varrido para encontrar o primeiro byte alterado, determinando o uso real de pilha.

### Valgrind massif (medição de heap)

O programa `massif_single.c` executa exatamente uma operação por invocação (sem laços) para que o Valgrind capture o perfil de heap daquela operação isolada. O delta líquido de heap é sempre zero — a liboqs libera todas as alocações ao término de cada chamada; o valor relevante é o **pico durante a execução**.

---

## Observação sobre Nomes de Algoritmos

A versão do liboqs utilizada ainda adota nomes internos pré-padronização. Os programas de memória aceitam tanto os nomes FIPS/NIST quanto os nomes internos:

| Nome NIST | Nome liboqs |
|---|---|
| FN-DSA-512 | Falcon-512 |
| FN-DSA-1024 | Falcon-1024 |
| SLH-DSA-SHAKE-128s | SPHINCS+-SHAKE-128s-simple |
| SLH-DSA-SHAKE-128f | SPHINCS+-SHAKE-128f-simple |
| SLH-DSA-SHAKE-192s | SPHINCS+-SHAKE-192s-simple |
| SLH-DSA-SHAKE-192f | SPHINCS+-SHAKE-192f-simple |
| SLH-DSA-SHAKE-256s | SPHINCS+-SHAKE-256s-simple |
| SLH-DSA-SHAKE-256f | SPHINCS+-SHAKE-256f-simple |
| ML-KEM-*, ML-DSA-*, HQC-* | (inalterado) |

---

## Compilar o Artigo (LaTeX)

```bash
cd ~/TCC_PQC/code
pdflatex tcc_pqc.tex
bibtex tcc_pqc
pdflatex tcc_pqc.tex
pdflatex tcc_pqc.tex
```

---

## Ambiente Testado

- **Hardware**: ASUS TUF Gaming A15 — AMD Ryzen 7 7435HS (8C/16T, AVX2, SHA, FMA3)
- **Memória**: 16 GB DDR5 4800 MHz dual-channel
- **SO**: Linux Mint 22.x (base Ubuntu 24.04, nativo)
- **Compilador**: GCC 13.3.0
- **liboqs**: 0.10.1

---

## Referências

- [liboqs — Open Quantum Safe](https://github.com/open-quantum-safe/liboqs)
- [NIST Post-Quantum Cryptography Standardization](https://csrc.nist.gov/projects/post-quantum-cryptography)
- FIPS 203 — ML-KEM (Module-Lattice-Based Key-Encapsulation Mechanism)
- FIPS 204 — ML-DSA (Module-Lattice-Based Digital Signature Algorithm)
- FIPS 205 — SLH-DSA (Stateless Hash-Based Digital Signature Algorithm)
- FIPS 206 — FN-DSA (FFT over NTRU-Lattice-Based Digital Signature Algorithm) — rascunho, ainda não finalizado

---

## Licença

Projeto desenvolvido para fins acadêmicos. A biblioteca liboqs é distribuída sob a licença MIT pelo projeto [Open Quantum Safe](https://openquantumsafe.org/).
