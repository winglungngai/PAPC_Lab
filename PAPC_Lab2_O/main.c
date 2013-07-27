#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

void merge(int *A, int *B, int *AB, int n, int m, int threads_num);
int rank(int a, int *sortedArray, int startPos, int endPos);
int compare_ints (const void *a, const void *b);
void mergeSeq_r(int *A, int *B, int *AB, int n, int m, int aPos, int bPos, int abPos);
void setup(int **A, int **B, int **AB, int n, int m);
void printDataset(char * description, int * dataset, int size);
void timeFunc(void (*f)(int *, int *, int *, int, int, int), int *A, int *B, int *AB, int n, int m, int threads_num, int repeat);
void mergeSeq(int *A, int *B, int *AB, int n, int m, int threads_num);

main ()
{
    int h,i,j,k;
    int n_var=7;
    int t_var=5;
    int nSize[7] = {16, 64, 256, 1024, 4096, 16384, 65536};
    int threads[5] = {1, 2, 4, 8, 16};
    int repeat = 1000;

    printf("Lab Work 2 - Performance Test - Merge By Ranking\n");
    printf("\t\tseq\t");
    for(h=0;h<t_var;h++)
        printf("t=%i\t", threads[h]);
    printf("\n");

    for(i=0; i<n_var; i++)
    {
        //set up inputs
        int *A, *B, *AB;
        int n = nSize[i]*2/3;
        int m = nSize[i]/3;
        setup(&A, &B, &AB, n, m);

        //printDataset("Input A: ", A, n);
        //printDataset("Input B: ", B, m);

        printf("size=%.5i\t", nSize[i]);

        timeFunc(mergeSeq, A, B, AB, n, m, 1, repeat);

        for(k=0;k<t_var;k++)
        {
            //set up threads number.
            int threads_num = threads[k];
            omp_set_dynamic(0);
            omp_set_num_threads(threads_num);

            timeFunc(merge, A, B, AB, n, m, threads_num, repeat);
        }
        printf("\n");

        //printDataset("Output AB: ", AB, n+m);
        //printf("\n");
    }
}

void timeFunc(void (*f)(int *, int *, int *, int, int, int), int *A, int *B, int *AB, int n, int m, int threads_num, int repeat)
{
    //start time measurement
    struct timeval startt, endt, result;
    int t;
    long timeDiff;
    result.tv_usec=0;
    gettimeofday (&startt, NULL);

    int l;
    for(l=0; l<repeat;l++)
        (*f)(A, B, AB, n, m, threads_num);

    //end time measurement
    gettimeofday (&endt, NULL);
    result.tv_usec = (endt.tv_sec*1000000+endt.tv_usec) - (startt.tv_sec*1000000+startt.tv_usec);
    timeDiff = result.tv_usec/repeat;

    //show elapsed time.
    printf("%03ld\t", timeDiff);
}

void printDataset(char * description, int * dataset, int size)
{
    int i;
    printf(description);
    for(i=0; i<size; i++)
        printf("%i ", dataset[i]);
    printf("\n");
}

void setup(int **A, int **B, int **AB, int n, int m)
{
    int j;

    *A = (int *) malloc(n*sizeof(int));
    *B = (int *) malloc(m*sizeof(int));
    *AB = (int *) malloc((n+m)*sizeof(int));

    srand (time(NULL));
    for(j=0; j<n; j++)
        (*A)[j]=rand() % 30;
    for(j=0; j<m; j++)
        (*B)[j]=rand() % 30;

    qsort (*A, n, sizeof (int), compare_ints);
    qsort (*B, m, sizeof (int), compare_ints);
}

void mergeSeq(int *A, int *B, int *AB, int n, int m, int threads_num)
{
    mergeSeq_r(A, B, AB, n, m, 0, 0, 0);
}

void mergeSeq_r(int *A, int *B, int *AB, int n, int m, int aPos, int bPos, int abPos)
{
    if(abPos < n+m)
    {
        if(aPos==n)
        {
            AB[abPos]=B[bPos];
            bPos++;
        }
        else if(bPos==m)
        {
            AB[abPos]=A[aPos];
            aPos++;
        }
        else
        {
            if(A[aPos]<B[bPos])
            {
                AB[abPos]=A[aPos];
                aPos++;
            }
            else
            {
                AB[abPos]=B[bPos];
                bPos++;
            }
        }
        abPos++;
        mergeSeq_r(A, B, AB, n, m, aPos, bPos, abPos);
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
