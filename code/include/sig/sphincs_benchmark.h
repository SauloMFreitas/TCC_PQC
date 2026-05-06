#ifndef SPHINCS_BENCHMARK_H
#define SPHINCS_BENCHMARK_H

#include "sig_benchmark.h"

/* SLH-DSA (ex-SPHINCS+) - FIPS 205
 *  SLH-DSA-SHAKE-128s/f | nivel 1
 *  SLH-DSA-SHAKE-192s/f | nivel 3
 *  SLH-DSA-SHAKE-256s/f | nivel 5
 * ATENCAO: Sign pode levar dezenas de ms por iteracao. */
void sphincs_benchmark_run_all(FILE *csv_fp, FILE *scale_csv_fp);

#endif /* SPHINCS_BENCHMARK_H */
