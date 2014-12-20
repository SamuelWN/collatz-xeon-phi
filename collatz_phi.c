#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

/* The __uint128_t type works in versions of gcc   */
/* such as 4.8 compiled for 64-bit platforms.      */
typedef unsigned long long bigInt;

//int printNum(bigInt num) {
//    unsigned long long low = (unsigned long long) num;
//    printf("%llu",low);
//}

/// bigSize = size_of(bigInt)
#define bigSize     1000000 //340282366920938463463374607431768211455

#define csvname     "results_1M.csv"
#define txtname     "results_1M_times.txt"


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
    bigInt i, j;
    double start, end;
//    to_ret (* restrict retlist) = malloc(sizeof(to_ret)*bigSize);
//    batcher (* restrict results) = malloc(sizeof(batcher)*bigSize);

//    to_ret retlist[bigSize]; ///Stores values as [num][#steps to smaller val][smaller val]
//    batcher results[bigSize]; ///Stores values as [#steps to smaller val][smaller val] & is sorted by num
    FILE *f = fopen(csvname, "w");
    FILE *t = fopen(txtname, "w");

    retlist[0].num = 1; retlist[0].to_batch.numSteps = 0; retlist[0].to_batch.stopPoint = 1;
    start = omp_get_wtime();
    
    int count;
    bigInt next;
    ///Calls checkNum for bigInt values
//    #pragma offload target(mic:0) inout(retlist:length(sizeof(to_ret))) shared(retlist) private(i, count, next)
    #pragma offload target(mic:0) inout(results) private(i, count, next)
    {
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
        results[retlist[i].num - 1] = retlist[i].to_batch;
    }
  }
    end = omp_get_wtime();
    fprintf(t, "Number of threads: %i\n", omp_get_max_threads());
    fprintf(t, "Start time = %f seconds\n", start);
    fprintf(t, "End   time = %f seconds\n", end);
    fprintf(t, "Run   time = %f seconds\n\n", (end - start));

    fprintf(f,"num,stopPoint,numSteps\n");

    /**
     * Process data and display results
     * (primarily for testing, can remove all but noted line for final version)
     */
    for(j = 0; j < bigSize; j++){
            /*fprintf(f, "num: ");
            fprintf(f,"%llu",(unsigned long long) j+1);

            fprintf(f, "\nresult[");
            fprintf(f,"%llu", (unsigned long long) j);
            fprintf(f, "].numSteps = %i\n", results[j].numSteps);

            fprintf(f, "result[");
            fprintf(f,"%llu", (unsigned long long) j);
            fprintf(f, "].stopPoint = ");
            fprintf(f,"%llu", (unsigned long long) results[j].stopPoint);
*/
        fprintf(f,"%llu,",(unsigned long long) j+1);
        fprintf(f, "%llu,", (unsigned long long) results[j].stopPoint);
        /************************************************************************
         *  Only line needed for final version:                                 *
         ************************************************************************/
            results[j].numSteps += results[results[j].stopPoint-1].numSteps;
        fprintf(f, "%i\n", results[j].numSteps);

        /*    fprintf(f, "\nresults[");
            fprintf(f,"%llu", (unsigned long long) j);
            fprintf(f, "].numSteps += <stopPoint> = %i\n\n", results[j].numSteps);*/
    }

    return(0);
}


