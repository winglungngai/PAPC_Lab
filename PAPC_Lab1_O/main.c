#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

void prefixMinima();
void printDataset();

main ()
{
    int i, n;


    //read input values from file;
    FILE *file = fopen("inputs/lab1/dataset3", "r");
    fscanf(file, "%i", &n);
    int A[n+1];
    A[0]=0;
    for (i= 1; i<n+1; i++)
        fscanf(file, "%i", &A[i]);

    //Define output
    int C[n];


    //show inputs
    //printDataset("Intput: ", &A, n+1);


    int k;
    for(k=1;k<=8;k++)
    {
        int threads_num = k;

        //run the method and measure time-complexity
        omp_set_dynamic(0);     // Explicitly disable dynamic teams
        omp_set_num_threads(threads_num); // Use 4 threads for all consecutive parallel regions

        clock_t begin, end;
        double time_spent;
        begin = clock();

        int repeat = 10000;
        for(i=0; i<repeat;i++)
            prefixMinima(n, &A, &C);

        end = clock();
        time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

        printf("Time spent: \t %.5f ms \n", time_spent/repeat*1000);
    }




    //show outputs
    //printDataset("Output: ", &C, n);

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

