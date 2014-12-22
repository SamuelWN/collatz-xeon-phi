/**
 * @author  Samuel Walters-Nevet
 * @brief   Collatz Calculator
 * @version 3.0
 * @file    collatz_phi.c
 * @note    This is designed for use on the Intel Xeon Phi MIC processor
 *
 * @note    This work is licensed under a Creative Commons
 *          Attribution-NonCommercial 4.0 International License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

typedef unsigned long long bigInt;

/// Number to test up to (starting from 1)
#define bigSize     1000000000 //340282366920938463463374607431768211455

#define csvname     "results_1B.csv"
#define txtname     "computation-time_100M.txt"


typedef struct {
    int numSteps;
    bigInt stopPoint;
} batcher;

typedef struct {
    bigInt num;
    batcher to_batch;
} to_ret;

__attribute__((target(mic))) to_ret retlist[bigSize]; ///Stores values as [num][#steps to smaller val][smaller val]
__attribute__((target(mic))) batcher results[bigSize]; ///Stores values as [#steps to smaller val][smaller val] & is sorted by num


int main () {
    bigInt j;
    double start, end;

    FILE *f = fopen(csvname, "w");
    FILE *t = fopen(txtname, "w");

    retlist[0].num = 1; retlist[0].to_batch.numSteps = 0; retlist[0].to_batch.stopPoint = 1;
    start = omp_get_wtime();
    
    #pragma offload target(mic:0) inout(results)
    {
        int count;
        bigInt i, next;

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
  }

    end = omp_get_wtime();
    fprintf(t, "Number of threads: %i\n", omp_get_max_threads());
    fprintf(t, "Start time = %f seconds\n", start);
    fprintf(t, "End   time = %f seconds\n", end);
    fprintf(t, "Run   time = %f seconds\n\n", (end - start));

    fprintf(f,"num,stopPoint,numSteps\n");

    /**
     * Process data and write out results
     */
    for(j = 0; j < bigSize; j++){
        results[j].numSteps += results[results[j].stopPoint-1].numSteps;

        fprintf(f,"%llu,",(unsigned long long) j+1);
        fprintf(f, "%llu,", (unsigned long long) results[j].stopPoint);
        fprintf(f, "%i\n", results[j].numSteps);
    }

    return(0);
}
