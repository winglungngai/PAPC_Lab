#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

void setup(int **S, int **R, int n);
void printDataset(char * description, int * dataset, int size);
void listRank(int *O, int *R, int n);
void timeFunc(void (*f)(int *, int *, int), int *S, int *R, int n, int repeat);
void listRankSeq(int *S, int *R, int n);
void verifyResult(int *seqResult, int *parResult, int n);

main ()
{
    int h,i,j,k;
    int n_var=7;
    int t_var=5;
    int nSize[7] = {16, 64, 256, 1024, 4096, 16384, 65536};
    //int nSize[7] = {17,17,17,17,17,17,17};
    int threads[5] = {1, 2, 4, 8, 16};
    int repeat = 1000;

    printf("Lab Work 3 - Performance Test - List Ranking\n");
    printf("\t\tseq\t");
    for(h=0;h<t_var;h++)
        printf("t=%i\t", threads[h]);
    printf("\n");

    for(i=0; i<n_var; i++)
    {
        //set up inputs
        int *S, *R;
        int n = nSize[i];
        setup(&S, &R, n);

//        int x[17] = {0, 14, 13, 5, 16, 11, 10, 9, 12, 0, 8, 7, 15, 4, 3, 2, 1};
//
//        S=x;

        //printf("Input n:  0  1  2  3  4\n");
        //printDataset("Input S: ", S, n);

        printf("size=%.5i\t", nSize[i]);

        timeFunc(listRankSeq, S, R, n, repeat);

        int seqResult[n];
        for(j=0;j<n;j++)
            seqResult[j] = R[j];

        for(k=0;k<t_var;k++)
        {
            //set up threads number.
            int threads_num = threads[k];
            omp_set_dynamic(0);
            omp_set_num_threads(threads_num);

            timeFunc(listRank, S, R, n, repeat);
        }

        int parResult[n];
        for(j=0;j<n;j++)
            parResult[j] = R[j];

        verifyResult(seqResult, parResult, n);
        printf("\n");





        //printDataset("Output R: ", R, n);
        //printf("\n");
    }
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


void listRank(int *O, int *R, int n)
{
    int i,j;
    int S[n];
    int SS[n];
    int RR[n];


    #pragma omp parallel shared(O, S, n) private(i)
    {
        #pragma omp for schedule(static) nowait
        for(i=0;i<n;i++)
        {
            S[i]=O[i];
            SS[i]=O[i];
        }
    }

    #pragma omp parallel shared(R, S, n) private(i)
    {
        #pragma omp for schedule(static)
        for(i=0;i<n;i++)
        {
            if(i != S[i])
            {
                R[i]=1;
                RR[i]=1;
            }
            else
            {
                R[i]=0;
                RR[i]=0;
            }
        }
    }

    for(j=0;j<(log (n) / log (2));j++)
    {
        #pragma omp parallel shared(R, S, n, RR, SS) default(none)
        {
            #pragma omp for schedule(static)
            for(i=0;i<n;i++)
            {
                if(S[i] != S[S[i]])
                {
                    RR[i] = R[i] + R[S[i]];
                    SS[i] = S[S[i]];
                }
            }

            #pragma omp for schedule(static)
            for(i=0;i<n;i++)
            {
                R[i] = RR[i];
                S[i] = SS[i];
            }
        }
    }
}

void listRankSeq(int *S, int *R, int n)
{
    int i;
    int V[n];

    //reverse direction
    for(i=0;i<n;i++)
        V[S[i]] = i;

    R[0]=0;
    int pos = 0;

    for(i=1;i<n;i++)
    {
        R[V[pos]]=i;
        pos = V[pos];
    }
}


void timeFunc(void (*f)(int *, int *, int), int *S, int *R, int n, int repeat)
{
    //start time measurement
    struct timeval startt, endt, result;
    int t;
    long timeDiff;
    result.tv_usec=0;
    gettimeofday (&startt, NULL);

    int l;
    for(l=0; l<repeat;l++)
        (*f)(S, R, n);

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
        printf("%2i ", dataset[i]);
    printf("\\\n");
}

void setup(int **S, int **R, int n)
{
    int i,j;

    *S = (int *) malloc(n*sizeof(int));
    *R = (int *) malloc(n*sizeof(int));

    int **seed = malloc(2 * sizeof(int*));
    for(i = 0; i < 2; i++)
        seed[i] = malloc(n * sizeof(int));

    for(i=0;i<n;i++)
        seed[0][i]=i;

    srand (time(NULL));
    for(j=1;j<n;j++)
    {
        int rPos1 = (rand() % (n-1))+1;
        int rPos2 = (rand() % (n-1))+1;
        int tmp = seed[0][rPos1];
        seed[0][rPos1] = (seed)[0][rPos2];
        seed[0][rPos2] = tmp;
    }

    seed[1][0] = seed[0][0];
    seed[1][1] = seed[0][0];
    for(j=2;j<n;j++)
    {
        seed[1][j] = seed[0][j-1];
    }

    for(i=0;i<n;i++)
    {
        (*S)[seed[0][i]]=seed[1][i];
    }

}
