#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

main ()
{

    int h,i,j;
    int n;
    int chunk = 1;
    int colSize, a, b;

    //read input values from file;
    FILE *file = fopen("inputs/lab1/dataset2", "r");

    fscanf(file, "%i", &n);
    int A[n+1];
    for(i=0; i<n+1; i++)
        A[i]=0;

    for (i= 1; i<n+1; i++)
    {
        fscanf(file, "%i", &A[i]);
    }

    //show inputs
    printf("Inputs: ");
    for (i= 1; i<n+1; i++)
    {
        printf("%i ", A[i]);
    }
    printf("\n");

    //init matrix B
    int rowCount = n;
    int colCount = (log (n) / log (2));
    printf(" rowCount : %i, columnCount: %i \n", rowCount, colCount);

    int B[colCount+1][rowCount+1];

    for(i=0; i<rowCount+1; i++)
        for(j=0; j<colCount+1; j++)
            B[j][i]=0;

    //step 1
    #pragma omp parallel shared(A, B, rowCount, chunk) private(i)
    {
        #pragma omp for schedule(dynamic,chunk) nowait
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
            #pragma omp for schedule(dynamic,chunk) nowait
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
            #pragma omp for schedule(dynamic,chunk) nowait
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
    int C[n];
    #pragma omp parallel shared(B, C, rowCount, chunk) private(i)
    {
        #pragma omp for schedule(dynamic,chunk) nowait
        for(i=0; i<n; i++)
        {
            C[i]=B[0][i+1];
        }
    }

    //show outputs
    printf("Outputs: ");
    for(i=0; i<n; i++)
    {
        printf("%i ", C[i]);
    }
    printf("\n");



//    //show results;
//    for(i=0; i<rowCount+1; i++)
//    {
//        for(j=0; j<colCount+1; j++)
//        {
//            printf("%i \t\t", B[j][i]);
//        }
//        printf("\n");
//    }
}


