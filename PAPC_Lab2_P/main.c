#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

int rank(int a, int *sortedArray, int startPos, int endPos);
int compare_ints (const void *a, const void *b);
void printDataset(char * description, int * dataset, int size);
void setup(int **A, int **B, int **AB, int n, int m);
void mergeSeq(int *A, int *B, int *AB, int n, int m);
void verifyResult(int *seqResult, int *parResult, int n);
void mergeSeq_r(int *A, int *B, int *AB, int n, int m, int aPos, int bPos, int abPos);

// Number of threads
#define NUM_THREADS 32

// Number of iterations
#define TIMES 1000

// Input Size
//#define NSIZE 1
#define NSIZE 7
#define NMAX 262144
int Ns[NSIZE] = {16, 64, 256, 1024, 4096, 16384, 65536};
//int Ns[NSIZE] = {10};

typedef struct __ThreadArg {
  int id;
  int nrT;
  int n,m;
  int *A, *B, *AB;
} tThreadArg;


pthread_t callThd[NUM_THREADS];
pthread_mutex_t mutexpm;
pthread_barrier_t barr, internal_barr;

void init(int n){
	/* Initialize the input for this iteration*/
	// B <- A
}

void seq_function(int m){
	/* The code for sequential algorithm */
	// Perform operations on B
}

void* par_function(void* a){
	/* The code for threaded computation */
    // Perform operations on B
    int t;
    for (t=0; t<TIMES; t++)
    {
        tThreadArg *tArg = (tThreadArg*) a;
        int *A = tArg->A;
        int *B = tArg->B;
        int *AB = tArg->AB;
        int tid = tArg->id;
        int n = tArg->n;
        int m = tArg->m;
        int thread_num = tArg->nrT;

        int i,j, RankA[n], RankB[m];
        int chunk, cStart, cEnd;

        chunk = n/thread_num+1;
        cStart = (n<chunk*(tid-1)) ? n : chunk*(tid-1);
        cEnd = (n<chunk*(tid)) ? n :chunk*(tid);

        for(i=cStart; i<cEnd; i++)
        {
           RankA[i] = rank(A[i]-1, B, 0, m-1);
           AB[i+RankA[i]] = A[i];
        }

        chunk = m/thread_num+1;
        cStart = (m<chunk*(tid-1)) ? m : chunk*(tid-1);
        cEnd = (m<chunk*(tid)) ? m :chunk*(tid);

        for(i=cStart; i<cEnd; i++)
        {
           RankB[i] = rank(B[i], A, 0, n-1);
           AB[i+RankB[i]] = B[i];
        }
        pthread_barrier_wait(&internal_barr);
    }
    pthread_barrier_wait(&barr);
}

void printDataset(char * description, int * dataset, int size)
{
    int i;
    printf(description);
    for(i=0; i<size; i++)
        printf("%i ", dataset[i]);
    printf("\n");
}

int main (int argc, char *argv[])
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

        int n=Ns[c]*2/3;
        int m=Ns[c]/3;

        // Seed Input
        int *A, *B, *AB;
        setup(&A, &B, &AB, n, m);

		printf("| %d | %d |",(n+m),TIMES);

		/* Run sequential algorithm */
		result.tv_usec=0;
		gettimeofday (&startt, NULL);

		for (t=0; t<TIMES; t++) {
            mergeSeq(A, B, AB, n, m);
		}
		gettimeofday (&endt, NULL);
		result.tv_usec = (endt.tv_sec*1000000+endt.tv_usec) - (startt.tv_sec*1000000+startt.tv_usec);
		printf("%ld | ", result.tv_usec/TIMES);

        int seqResult[n+m];
        for(j=0;j<n+m;j++)
            seqResult[j] = AB[j];

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

			result.tv_sec=0; result.tv_usec=0;
			for (j=1; j<=/*NUMTHRDS*/nt; j++)
        		{
				x[j].id = j;
				x[j].nrT=nt; // number of threads in this round
				x[j].n=n;  //input size
				x[j].m=m;   //input size
				x[j].A = A;
				x[j].B = B;
				x[j].AB = AB;
				pthread_create(&callThd[j-1], &attr, par_function, (void *)&x[j]);
			}

			gettimeofday (&startt, NULL);

			pthread_barrier_wait(&barr);
			gettimeofday (&endt, NULL);

			//printDataset("\nA: ", A, n);
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
        int parResult[n+m];
        for(j=0;j<n+m;j++)
            parResult[j] = AB[j];
        verifyResult(seqResult, parResult, n);
		printf("\n");

            //printDataset("\nseqResult: ", seqResult, n+m);
			//printDataset("\nparResult: ", parResult, n+m);
	}
	pthread_exit(NULL);
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


void mergeSeq(int *A, int *B, int *AB, int n, int m)
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

}
