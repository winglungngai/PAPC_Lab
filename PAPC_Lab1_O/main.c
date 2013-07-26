#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

void prefixMinima();
void printDataset();

main ()
{
    int h,i,j,k,l;
    int nSize[7] = {16, 64, 256, 1024, 4096, 16384, 65536};
    int threads[5] = {1, 2, 4, 8, 16};
    int repeat = 1000;

    printf("Lab Work 1 - Performance Test\n");
    printf("\t\t");
    for(h=0;h<5;h++)
    {
        printf("t=%i\t", threads[h]);
    }
    printf("\n");

    for(i=0; i<7; i++)
    {
        int n;
        //set up inputs
        n = nSize[i];
        int A[n], C[n];

        //set up inputs, A[0] is set to 0 as it should be empty.
        A[0]=0;
        srand (time(NULL));
        for(j=0; j<n; j++)
            A[i]=rand() % 300;

        //show inputs
        //printDataset("Intput: ", &A, n+1);

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
                prefixMinima(n, &A, &C);

            //end time measurement
            gettimeofday (&endt, NULL);
            result.tv_usec = (endt.tv_sec*1000000+endt.tv_usec) - (startt.tv_sec*1000000+startt.tv_usec);
            long timeDiff = result.tv_usec/repeat;

            //show elapsed time.
            printf("%03ld\t", timeDiff);
        }
        printf("\n");

        //show outputs
        //printDataset("Output: ", &C, n);
    }

}

void printDataset(char * description, int * dataset, int size)
{
    int i;
    printf(description);
    for(i=0; i<size; i++)
        printf("%i ", dataset[i]);
    printf("\n");
}

void prefixMinima(int n, int *A, int *C)
{
    int h,i,j;

    int chunk = 128;
    int colSize, a, b;

    //init matrix B
    int rowCount = n;
    int colCount = (log (n) / log (2));
    //printf(" rowCount : %i, columnCount: %i \n", rowCount, colCount);

    int B[colCount+1][rowCount+1];

    //step 1
    #pragma omp parallel shared(A, B, rowCount, chunk) private(i)
    {
        #pragma omp for schedule(static,chunk) nowait
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
        #pragma omp parallel shared(A, B, h, colSize, chunk) private(j,a,b)
        {
            colSize = (int) (n / pow((double) 2,h));
            #pragma omp for schedule(static,chunk) nowait
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
        #pragma omp parallel shared(A, B, h, colSize, chunk) private(j,a,b)
        {
            int colSize = (int) (n / pow((double) 2,h));
            #pragma omp for schedule(static,chunk) nowait
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
    #pragma omp parallel shared(B, C, rowCount, chunk) private(i)
    {
        #pragma omp for schedule(static,chunk) nowait
        for(i=0; i<n; i++)
        {
            C[i]=B[0][i+1];
        }
    }
}

