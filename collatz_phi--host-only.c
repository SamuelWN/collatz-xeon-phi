/**
 * @author  Samuel Walters-Nevet
 * @brief   Collatz Calculator
 * @version 3.0
 * @file    collatz_phi--host-only.c
 *
 * @note    This work is licensed under a Creative Commons
 *          Attribution-NonCommercial 4.0 International License.
 */
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

typedef unsigned long bigInt;

/// Number to test up to
#define bigSize     100000

#define csvname     "results_100M_32b_03threads.csv"
#define txtname     "results_100K_times_Xthread_local.txt"


typedef struct {
    int numSteps;
    bigInt stopPoint;
} batcher;

typedef struct {
    bigInt num;
    batcher to_batch;
} to_ret;

static to_ret retlist[bigSize]; ///Stores values as [num][#steps to smaller val][smaller val]
static batcher results[bigSize]; ///Stores values as [#steps to smaller val][smaller val] & is sorted by num


int main () {
    bigInt i, j;
    double start, end;

    FILE *f = fopen(csvname, "w");
    FILE *t = fopen(txtname, "w");

    retlist[0].num = 1; retlist[0].to_batch.numSteps = 0; retlist[0].to_batch.stopPoint = 1;
    start = omp_get_wtime();

    int count;
    bigInt next;

    #pragma omp parallel for
    for(i = 1; i < bigSize; i++){
        next = retlist[i].num = i + 1;
        count = 0;

        do {
            count++;

            if (next%2 == 1)
                next=(3*next+1)/2;
            else
                next/=2;

        } while(next > retlist[i].num);

        retlist[i].to_batch.numSteps = count;
        retlist[i].to_batch.stopPoint = next;
    }

    ///Organizes data into a sorted array
    #pragma omp parallel for
    for (i = 0; i < bigSize; i++){
        results[i] = retlist[i].to_batch;
    }

    end = omp_get_wtime();
    fprintf(t, "Number of threads: %i\n", omp_get_max_threads());
    fprintf(t, "Start time = %f seconds\n", start);
    fprintf(t, "End   time = %f seconds\n", end);
    fprintf(t, "Run   time = %f seconds\n\n", (end - start));

    fprintf(f,"num,stopPoint,numSteps\n");

    /**                                    **
     * Process data and write out results   *
     **                                    **/
    for(j = 0; j < bigSize; j++){
        results[j].numSteps += results[results[j].stopPoint-1].numSteps;

        fprintf(f,"%llu,",(unsigned long long) j+1);
        fprintf(f, "%llu,", (unsigned long long) results[j].stopPoint);
        fprintf(f, "%i\n", results[j].numSteps);
    }

    return(0);
}


