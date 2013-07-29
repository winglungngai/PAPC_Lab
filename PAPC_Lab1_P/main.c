#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

void printDataset();
int performanceTest(int minType);
void suffixMinimaSeq(int n, int *A, int *D);
void prefixMinimaSeq(int n, int *A, int *C);
void setup(int **A, int **B, int **C, int n);
void verifyResult(int *seqResult, int *parResult, int n);
void chunckInto(int *chunk,int *cStart,int *cEnd,int n, int tid, int thread_num);
void* prefixMinima(void* a);
void* suffixMinima(void* a);

// Number of threads
#define NUM_THREADS 32
//#define NUM_THREADS 4

// Number of iterations
//#define TIMES 1
#define TIMES 1000

// Input Size
#define NSIZE 7
//#define NSIZE 1
#define NMAX 262144
int Ns[NSIZE] = {16, 64, 256, 1024, 4096, 16384, 65536};
//int Ns[NSIZE] = {8};

typedef struct __ThreadArg {
  int id;
  int nrT;
  int n,m;
  int *A, *C, **B;
} tThreadArg;


pthread_t callThd[NUM_THREADS];
pthread_mutex_t mutexpm;
pthread_barrier_t barr, internal_barr;



int main (int argc, char *argv[])
{
    performanceTest(1);
    performanceTest(2);
    pthread_exit(NULL);
}

int performanceTest(int minType)
{
    struct timeval startt, endt, result;
	int i, j, k, nt, t, n, c;
	void *status;
   	pthread_attr_t attr;
  	tThreadArg x[NUM_THREADS];

  	result.tv_sec = 0;
  	result.tv_usec= 0;


   	/* Initialize and set thread detached attribute */
   	pthread_attr_init(&attr);
   	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	printf("|NSize|Iterations|Seq|Th01|Th02|Th04|Th08|Par16|\n");

	// for each input size
	for(c=0; c<NSIZE; c++){

        int n=Ns[c];

        // Seed Input
        int *A, *C, *B;
        setup(&A, &B, &C, n);
        int rowCount = n;
        int colCount = (log (n) / log (2));
        //int B[colCount+1][rowCount+1];

		printf("| %d | %d |",n,TIMES);

		/* Run sequential algorithm */
		result.tv_usec=0;
		gettimeofday (&startt, NULL);

		for (t=0; t<TIMES; t++) {
            if(minType==1)
                prefixMinimaSeq(n, A, C);
            else if (minType==2)
                suffixMinimaSeq(n, A, C);
		}

		gettimeofday (&endt, NULL);
		result.tv_usec = (endt.tv_sec*1000000+endt.tv_usec) - (startt.tv_sec*1000000+startt.tv_usec);
		printf("%ld | ", result.tv_usec/TIMES);

        int seqResult[n];
        for(j=0;j<n;j++)
            seqResult[j] = C[j];



		/* Run threaded algorithm(s) */
		for(nt=1; nt<NUM_THREADS; nt=nt<<1){

		        if(pthread_barrier_init(&barr, NULL, nt+1))
    			{
        			printf("Could not create a barrier\n");
			        return -1;
                }

		        if(pthread_barrier_init(&internal_barr, NULL, nt))
    			{
        			printf("Could not create a barrier\n");
			        return -1;
                }

            //printf("section with nt=%i\n", nt);
			result.tv_sec=0; result.tv_usec=0;
			for (j=1; j<=/*NUMTHRDS*/nt; j++)
        		{
				x[j].id = j;
				x[j].nrT=nt; // number of threads in this round
				x[j].n=n;  //input size
				x[j].A = A;
				x[j].B = B;
				x[j].C = C;
                if(minType==1)
                    pthread_create(&callThd[j-1], &attr, prefixMinima, (void *)&x[j]);
                else if (minType==2)
                    pthread_create(&callThd[j-1], &attr, suffixMinima, (void *)&x[j]);
			}

			gettimeofday (&startt, NULL);

			pthread_barrier_wait(&barr);
			gettimeofday (&endt, NULL);

			//printDataset("\nA: ", A, n+1);
            //printDataset("B: ", B, m);
            //printDataset("AB: ", AB, n+m);


			/* Wait on the other threads */
			for(j=0; j</*NUMTHRDS*/nt; j++)
			{
				pthread_join(callThd[j], &status);
			}

			if (pthread_barrier_destroy(&barr)) {
        			printf("Could not destroy the barrier\n");
			        return -1;
			}
			if (pthread_barrier_destroy(&internal_barr)) {
        			printf("Could not destroy the barrier\n");
			        return -1;
			}
   			result.tv_usec += (endt.tv_sec*1000000+endt.tv_usec) - (startt.tv_sec*1000000+startt.tv_usec);
            printf("%ld | ", result.tv_usec/TIMES);
		}
        int parResult[n];
        for(j=0;j<n;j++)
            parResult[j] = C[j];
        verifyResult(seqResult, parResult, n);
		printf("\n");

            //printDataset("\nseqResult: ", seqResult, n);
			//printDataset("\nparResult: ", parResult, n);
	}


}


void chunckInto(int *chunk,int *cStart,int *cEnd,int n, int tid, int thread_num)
{
    *chunk = n/thread_num+1;
    *cStart = (n<(*chunk)*(tid-1)) ? n : (*chunk)*(tid-1);
    *cEnd = (n<(*chunk)*(tid)) ? n :(*chunk)*(tid);
}

void* prefixMinima(void* a){
	/* The code for threaded computation */
    // Perform operations on B
    int t;
    int h,i,j;
    for (t=0; t<TIMES; t++)
    {
        tThreadArg *tArg = (tThreadArg*) a;
        int *A = tArg->A;
        int *C = tArg->C;
        int **B = tArg->B;
        int tid = tArg->id;
        int n = tArg->n;
        int thread_num = tArg->nrT;

        int colSize, a, b;

        //init matrix B
        int rowCount = n;
        int colCount = (log (n) / log (2));

        int chunk, cStart, cEnd;

        chunckInto(&chunk, &cStart, &cEnd, rowCount+1, tid, thread_num);
        //step 1
        for(i=cStart; i<cEnd; i++)
        {
          B[0][i] = A[i];
        }
        pthread_barrier_wait(&internal_barr);

        //step 2
        for(h=1;h<=colCount;h++)
        {
            colSize = (int) (n / pow((double) 2,h));
            chunckInto(&chunk, &cStart, &cEnd, colSize+1, tid, thread_num);
            for(j=cStart;j<cEnd;j++)
            {   if(j!=0)
                {
                    //printf("cS=%i, cE=%i, t=%i,  colSize=%i, h= %i,j=%i, gather %i, %i\n", cStart, cEnd, tid, colSize, h, j, 2*j-1,2*j);
                    a = B[(h-1)][(2*j-1)];
                    b = B[(h-1)][(2*j)];
                    if(a <= b)
                        B[h][j] = a;
                    else
                        B[h][j] = b;
                }

            }
            pthread_barrier_wait(&internal_barr);
        }
        pthread_barrier_wait(&internal_barr);

        //step 3
        for(h=colCount;h>=0;h--)
        {
                int colSize = (int) (n / pow((double) 2,h));
                chunckInto(&chunk, &cStart, &cEnd, colSize+1, tid, thread_num);
                for(j=cStart;j<cEnd;j++)
                {
                    if(j!=0)
                    {
                        //printf("cS=%i, cE=%i, t=%i,  colSize=%i, h= %i,j=%i, gather %i, %i\n", cStart, cEnd, tid, colSize, h, j, 2*j-1,2*j);

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
                pthread_barrier_wait(&internal_barr);
        }
        pthread_barrier_wait(&internal_barr);

        chunckInto(&chunk, &cStart, &cEnd, n, tid, thread_num);
        for(i=cStart; i<cEnd; i++)
        {
            C[i]=B[0][i+1];
        }
        pthread_barrier_wait(&internal_barr);
    }
    pthread_barrier_wait(&barr);

}


void* suffixMinima(void* a){
	/* The code for threaded computation */
    // Perform operations on B
    int t;
    int h,i,j;
    for (t=0; t<TIMES; t++)
    {
        tThreadArg *tArg = (tThreadArg*) a;
        int *A = tArg->A;
        int *C = tArg->C;
        int **B = tArg->B;
        int tid = tArg->id;
        int n = tArg->n;
        int thread_num = tArg->nrT;

        int colSize, a, b;
        //init matrix B
        int rowCount = n;
        int colCount = (log (n) / log (2));

        int chunk, cStart, cEnd;

        chunckInto(&chunk, &cStart, &cEnd, rowCount+1, tid, thread_num);
        //step 1
        for(i=cStart; i<cEnd; i++)
        {
          B[0][i] = A[(n+1)-i];
        }
        pthread_barrier_wait(&internal_barr);

        //step 2
        for(h=1;h<=colCount;h++)
        {
            colSize = (int) (n / pow((double) 2,h));
            chunckInto(&chunk, &cStart, &cEnd, colSize+1, tid, thread_num);
            for(j=cStart;j<cEnd;j++)
            {   if(j!=0)
                {
                    //printf("cS=%i, cE=%i, t=%i,  colSize=%i, h= %i,j=%i, gather %i, %i\n", cStart, cEnd, tid, colSize, h, j, 2*j-1,2*j);
                    a = B[(h-1)][(2*j-1)];
                    b = B[(h-1)][(2*j)];
                    if(a <= b)
                        B[h][j] = a;
                    else
                        B[h][j] = b;
                }

            }
            pthread_barrier_wait(&internal_barr);
        }
        pthread_barrier_wait(&internal_barr);

            //step 3
        for(h=colCount;h>=0;h--)
        {
                int colSize = (int) (n / pow((double) 2,h));
                chunckInto(&chunk, &cStart, &cEnd, colSize+1, tid, thread_num);
                for(j=cStart;j<cEnd;j++)
                {
                    if(j!=0)
                    {
                        //printf("cS=%i, cE=%i, t=%i,  colSize=%i, h= %i,j=%i, gather %i, %i\n", cStart, cEnd, tid, colSize, h, j, 2*j-1,2*j);

                        if((j%2)==0)
                        {
                            //printf("DDDDDDDDDDDDDDDDDDDDDD");
                            B[h][j] = B[h+1][j/2];
                            //B[h][j]=-99*10-h;
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
                            //B[h][j] = -99;

                        }
                    }

                }
                pthread_barrier_wait(&internal_barr);
        }
        pthread_barrier_wait(&internal_barr);

        //step 4
        chunckInto(&chunk, &cStart, &cEnd, n, tid, thread_num);
        for(i=cStart; i<cEnd; i++)
        {
            C[i]=B[0][n-i];
        }
        pthread_barrier_wait(&internal_barr);
    }
    pthread_barrier_wait(&barr);

}

void setup(int **A, int **B, int **C, int n)
{
    int i,j;
    *A = (int *) malloc((n+1)*sizeof(int));
    *C = (int *) malloc(n*sizeof(int));

    srand (time(NULL));
    (*A)[0]=0;
    for(j=1; j<n+1; j++)
        (*A)[j]=rand() % 300;

    int rowCount = n;
    int colCount = (log (n) / log (2));

//    *B = malloc((4) * sizeof(int*));
//    for(i = 0; i < (4); i++)
//        (*B)[i] = malloc((9) * sizeof(int));

    *B = malloc((colCount+1) * sizeof(int*));
    for(i = 0; i < (colCount+1); i++)
        (*B)[i] = malloc((rowCount+1) * sizeof(int));
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
