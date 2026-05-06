#ifndef STATS_H
#define STATS_H

/*
 * stats.h - Estatisticas descritivas para amostras de benchmark
 */

typedef struct {
    double mean_us;     /* media em microssegundos   */
    double stddev_us;   /* desvio padrao             */
    double min_us;      /* minimo observado          */
    double max_us;      /* maximo observado          */
    double median_us;   /* mediana                   */
    double cv_percent;  /* coeficiente de variacao % */
} stats_t;

/*
 * Calcula estatisticas a partir de amostras em microssegundos.
 * O array original nao e modificado.
 *
 * @param samples  Array de tempos em us (retornados por timer_us())
 * @param n        Numero de amostras
 * @param out      Estrutura de saida
 */
void stats_compute(const double *samples, int n, stats_t *out);

/*
 * Imprime resumo formatado no stdout.
 */
void stats_print(const char *label, const stats_t *s);

#endif /* STATS_H */
