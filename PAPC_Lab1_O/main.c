#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <math.h>

void prefixMinima();
void suffixMinima();
void printDataset();
void performanceTest();
void suffixMinimaSeq(int n, int *A, int *D);
void prefixMinimaSeq(int n, int *A, int *C);
void setup(int **A, int **C, int n);
void timeFunc(void (*f)(int, int *, int *), int n, int *A, int *C, int repeat);
void verifyResult(int *seqResult, int *parResult, int n);

main ()
{
    performanceTest(1);
    performanceTest(2);
}

void performanceTest(int minType)
{
    int h,i,j,k;
    int n_var=7;
    int t_var=5;
    int nSize[7] = {16, 64, 256, 1024, 4096, 16384, 65536};
    int threads[5] = {1, 2, 4, 8, 16};
    int repeat = 1000;

    if(minType==1)
        printf("Lab Work 1 - Performance Test - PrefixMinima\n");
    else if (minType==2)
        printf("Lab Work 1 - Performance Test - SuffixMinima\n");

    printf("\t\tseq\t");
    for(h=0;h<t_var;h++)
        printf("t=%i\t", threads[h]);
    printf("\n");

    for(i=0; i<n_var; i++)
    {
        //set up inputs
        int n;
        n = nSize[i];
        int *A, *C;
        setup(&A,&C, n);

        //printDataset("Intput: ", A, n+1);

        printf("size=%.5i\t", n);

        if(minType==1)
            timeFunc(prefixMinimaSeq, n, A, C, repeat);
        else if (minType==2)
            timeFunc(suffixMinimaSeq, n, A, C, repeat);

        int seqResult[n];
        for(j=0;j<n;j++)
            seqResult[j] = C[j];

        for(k=0;k<5;k++)
        {
            //set up threads number.
            int threads_num = threads[k];
            omp_set_dynamic(0);
            omp_set_num_threads(threads_num);

            if(minType==1)
                timeFunc(prefixMinima, n, A, C, repeat);
            else if (minType==2)
                timeFunc(suffixMinima, n, A, C, repeat);
        }
        int parResult[n];
        for(j=0;j<n;j++)
            parResult[j] = C[j];
        verifyResult(seqResult, parResult, n);
        printf("\n");

        //printDataset("Output: ", C, n);
        //printf("\n");
    }
}


void setup(int **A, int **C, int n)
{
    int j;
    *A = (int *) malloc((n+1)*sizeof(int));
    *C = (int *) malloc(n*sizeof(int));

    srand (time(NULL));
    (*A)[0]=0;
    for(j=1; j<n+1; j++)
        (*A)[j]=rand() % 300;
}

void timeFunc(void (*f)(int, int *, int *), int n, int *A, int *C, int repeat)
{
    //start time measurement
    struct timeval startt, endt, result;
    int t;
    long timeDiff;
    result.tv_usec=0;
    gettimeofday (&startt, NULL);

    int l;
    for(l=0; l<repeat;l++)
        (*f)(n, A, C);

    //end time measurement
    gettimeofday (&endt, NULL);
    result.tv_usec = (endt.tv_sec*1000000+endt.tv_usec) - (startt.tv_sec*1000000+startt.tv_usec);
    timeDiff = result.tv_usec/repeat;

    //show elapsed time.
    printf("%ld\t", timeDiff);
}

void verifyResult(int *seqResult, int *parResult, int n)
{
    int identical = 1;
    int i;
    for(i=0;i<n;i++)
    {
        if(seqResult[i] != parResult[i])
            identical = 0;
    }
    if(identical)
    {
        printf("seq=par");
    }
    else
    {
        printf("seq!=par");
    }

//    if(n<64)
//    {
//        printDataset("\nseq: ", seqResult, n);
//        printDataset("par: ", parResult, n);
//    }
}

void printDataset(char * description, int * dataset, int size)
{
    int i;
    printf(description);
    for(i=0; i<size; i++)
        printf("%i ", dataset[i]);
    printf("\n");
}

void prefixMinimaSeq(int n, int *A, int *C)
{
    int i;
    const int swiftPos = 1;

    C[0]=A[0+swiftPos];

    for(i=1; i<n; i++)
    {
        if(C[i-1]<A[i+swiftPos])
            C[i]=C[i-1];
        else
            C[i]=A[i+swiftPos];
    }
}

void suffixMinimaSeq(int n, int *A, int *D)
{
    int i;
    const int swiftPos = 1;

    D[n-1]=A[n-1+swiftPos];

    for(i=n-2; i>=0; i--)
    {
        if(D[i+1]<A[i+swiftPos])
            D[i]=D[i+1];
        else
            D[i]=A[i+swiftPos];
    }
}



void suffixMinima(int n, int *A, int *D)
{
    int i;
    int B[n+1], C[n];
    B[0]=0;

    #pragma omp parallel shared(A, B, n) private(i)
    {
        #pragma omp for schedule(static) nowait
        for(i=1;i<n+1;i++)
        {
            B[i]=A[(n+1)-i];
        }
    }

    prefixMinima(n, &B, &C);

    #pragma omp parallel shared(C, D, n) private(i)
    {
        #pragma omp for schedule(static) nowait
        for(i=0;i<n;i++)
        {
            D[i]=C[(n-1)-i];
        }
    }

}

void prefixMinima(int n, int *A, int *C)
{
    int h,i,j;

    int colSize, a, b;

    //init matrix B
    int rowCount = n;
    int colCount = (log (n) / log (2));
    //printf(" rowCount : %i, columnCount: %i \n", rowCount, colCount);

    int B[colCount+1][rowCount+1];

    //step 1
    #pragma omp parallel shared(A, B, rowCount) private(i)
    {
        #pragma omp for schedule(static) nowait
        for(i=0; i<rowCount+1; i++)
        {
//            int ID = omp_get_thread_num();
//            printf("i=%i, thread=%i \n", i, ID);
            B[0][i] = A[i];
        }
    }


    //step 2
    for(h=1;h<=colCount;h++)
    {
        #pragma omp parallel shared(A, B, h, colSize) private(j,a,b)
        {
            colSize = (int) (n / pow((double) 2,h));
            #pragma omp for schedule(static) nowait
            for(j=1;j<=colSize;j++)
            {
//                int ID = omp_get_thread_num();
//                printf("h=%i, j=%i, thread=%i \n", h, j, ID);

                a = B[(h-1)][(2*j-1)];
                b = B[(h-1)][(2*j)];
                if(a <= b)
                    B[h][j] = a;
                else
                    B[h][j] = b;
            }
        }
    }


    //step 3
    for(h=colCount;h>=0;h--)
    {
        #pragma omp parallel shared(A, B, h, colSize) private(j,a,b)
        {
            int colSize = (int) (n / pow((double) 2,h));
            #pragma omp for schedule(static) nowait
            for(j=1;j<=colSize+1;j++)
            {
                if((j%2)==0)
                {
                    B[h][j] = B[h+1][j/2];
                }
                else if (j==1)
                {
                }
                else
                {
                    int a = B[h][j];
                    int b = B[h+1][(j-1)/2];
                    if(a <= b)
                        B[h][j] = a;
                    else
                        B[h][j] = b;

                }
            }
        }
    }

    //step 4
    #pragma omp parallel shared(B, C, rowCount) private(i)
    {
        #pragma omp for schedule(static) nowait
        for(i=0; i<n; i++)
        {
            C[i]=B[0][i+1];
        }
    }
}

