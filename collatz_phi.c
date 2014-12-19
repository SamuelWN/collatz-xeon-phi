#ifndef MIC_DEV
#define MIC_DEV 0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>

typedef __uint64_t bigInt;

/// bigSize = size_of(bigInt)
#define bigSize     100000000 //340282366920938463463374607431768211455

typedef struct {
    int numSteps;
    bigInt stopPoint;
} batcher;

typedef struct {
    bigInt num;
    batcher to_batch;
} to_ret;


void checkNum(to_ret (* restrict retlist))
{
#pragma offload target(mic:MIC_DEV) \
                inout(retlist:length(bigSize))
  {
    bigInt i;
    int count;

#pragma omp parallel for shared(retlist) private(i)
    for (i = 0; i < bigSize; ++i){
        bigInt next = retlist[i].num = i + 1;

        for(count = 0; next >= retlist[i].num; count++){
            if (next%2 == 0)
                next/=2;
            else
                next=3*next+1;
        }

        retlist[i].to_batch.numSteps = count;
        retlist[i].to_batch.stopPoint = next;
    }
  }
}

int main()
{
    bigInt i, j;
    double start, end;
    FILE *f = fopen("results_phi_calc_10-8.txt", "w");

    to_ret (* restrict retlist) = malloc(sizeof(to_ret)*bigSize);
    batcher (* restrict results) = malloc(sizeof(batcher)*bigSize);

    start = omp_get_wtime();
    checkNum(retlist);

    #pragma omp parallel for shared(retlist, results)
    for (i = 0; i < bigSize; i++) {
        results[retlist[i].num - 1] = retlist[i].to_batch;
    }

    end = omp_get_wtime();
    end -= start;
    printf("Runtime = %d seconds\n", end);

    for(j = 0; j < bigSize; j++){
        fprintf(f, "num: ");
        fprintf(f,"%llu",(unsigned long long) j+1);

        fprintf(f, "\nresult[");
        fprintf(f,"%llu", (unsigned long long) j);
        fprintf(f, "].numSteps = %i\n", results[j].numSteps);

        fprintf(f, "result[");
        fprintf(f,"%llu", (unsigned long long) j);
        fprintf(f, "].stopPoint = ");
        fprintf(f,"%llu", (unsigned long long) results[j].stopPoint);

    /************************************************************************
     *  Only line needed for final version:                                 *
     ************************************************************************/
        results[j].numSteps += results[results[j].stopPoint-1].numSteps;

        fprintf(f, "\nresults[");
        fprintf(f,"%llu", (unsigned long long) j);
        fprintf(f, "].numSteps += <stopPoint> = %i\n\n", results[j].numSteps);
    }

  free(retlist); free(results);
  return 0;
}



