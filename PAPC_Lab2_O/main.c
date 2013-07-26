#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

int rank(int a, int *sortedArray, int startPos, int endPos);
int compare_ints (const void *a, const void *b);
void merge();

main ()
{
    int i,m,n;

    //set up inputs
    n = 10000;
    m = 20000;
    int A[n], B[m], AB[n+m];

    srand (time(NULL));
    for(i=0; i<n; i++)
        A[i]=rand() % 30;
    for(i=0; i<m; i++)
        B[i]=rand() % 30;

    qsort (A, n, sizeof (int), compare_ints);
    qsort (B, m, sizeof (int), compare_ints);

    //show inputs
//    printf("A: ");
//    for(i=0; i<n; i++)
//        printf("%i ", A[i]);
//    printf("\n");
//
//    printf("B: ");
//    for(i=0; i<m; i++)
//        printf("%i ", B[i]);
//    printf("\n");

    int k;
    for(k=1;k<=8;k++)
    {
        int threads_num = k;

        //run the method and measure time-complexity
        omp_set_dynamic(0);
        omp_set_num_threads(threads_num);

        clock_t begin, end;
        double time_spent;
        begin = clock();

        int repeat = 1000;
        for(i=0; i<repeat;i++)
            merge(A, B, AB, n, m, threads_num);

        end = clock();
        time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

        printf("threads_num: %i \t %.5f ms \n", threads_num, time_spent/repeat*1000);
    }

    //show outputs
//    printf("AB: ");
//    for(i=0; i<m+n; i++)
//        printf("%i ", AB[i]);
//    printf("\n");
}

void merge(int *A, int *B, int *AB, int n, int m, int threads_num)
{
    int i, RankA[n], RankB[m];
    int chunk;

    chunk = n/threads_num + 1;

    #pragma omp parallel shared(A, B, m, n, RankA, AB) private(i)
    {
        #pragma omp for schedule(static,chunk) nowait
        for(i=0; i<n; i++)
        {
           RankA[i] = rank(A[i]-1, B, 0, m-1);
           AB[i+RankA[i]] = A[i];
        }
    }

    chunk = m/threads_num + 1;

    #pragma omp parallel shared(A, B, m, n, RankA, AB) private(i)
    {
        #pragma omp for schedule(static,chunk) nowait
        for(i=0; i<m; i++)
        {
            RankB[i] = rank(B[i], A, 0, n-1);
            AB[i+RankB[i]] = B[i];
        }
    }

}

int rank(int a, int *sortedArray, int startPos, int endPos)
{
    int midPos = startPos + (endPos-startPos)/2;

    if ((endPos-startPos)<=0)
    {
        if(sortedArray[midPos]<=a)
            return midPos+1;
        else
            return midPos;
    }

    //printf("a=%i, startPos=%i, midPos=%i, endPos=%i\n", a, startPos, midPos, endPos);
    if(a<sortedArray[midPos])
        return rank(a, sortedArray, startPos, midPos-1);
    else
        return rank(a, sortedArray, midPos+1, endPos);
}


int compare_ints (const void *a, const void *b)
{
    const int *da = (const int *) a;
    const int *db = (const int *) b;

    return (*da > *db) - (*da < *db);
}
