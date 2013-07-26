#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

void merge(int *A, int *B, int *AB, int n, int m, int threads_num);
int rank(int a, int *sortedArray, int startPos, int endPos);
int compare_ints (const void *a, const void *b);

main ()
{
    int h,i,j,k,l;
    int nSize[7] = {16, 64, 256, 1024, 4096, 16384, 65536};
    int threads[5] = {1, 2, 4, 8, 16};
    int repeat = 1000;

    printf("\t\t");
    for(h=0;h<5;h++)
    {
        printf("t=%i\t", threads[h]);
    }
    printf("\n");

    for(i=0; i<7; i++)
    {
        int m,n;
        //set up inputs
        n = nSize[i];
        m = nSize[i]*2;
        int A[n], B[m], AB[n+m];

        srand (time(NULL));
        for(j=0; j<n; j++)
            A[i]=rand() % 30;
        for(j=0; j<m; j++)
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

        printf("size=%.5i\t", nSize[i]);
        for(k=0;k<5;k++)
        {
            //set up threads number.
            int threads_num = threads[k];
            omp_set_dynamic(0);
            omp_set_num_threads(threads_num);

            //start time measurement
            struct timeval startt, endt, result;
            int t;
            int TIMES = 1000000;
            result.tv_usec=0;
            gettimeofday (&startt, NULL);

            //run parellel algorithm
            for(l=0; l<repeat;l++)
                merge(A, B, AB, n, m, threads_num);

            //end time measurement
            gettimeofday (&endt, NULL);
            result.tv_usec = (endt.tv_sec*1000000+endt.tv_usec) - (startt.tv_sec*1000000+startt.tv_usec);
            long timeDiff = result.tv_usec/repeat;

            //show elapsed time.
            printf("%03ld\t", timeDiff);
        }
        printf("\n");

        //show outputs
    //    printf("AB: ");
    //    for(i=0; i<m+n; i++)
    //        printf("%i ", AB[i]);
    //    printf("\n");
    }
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
