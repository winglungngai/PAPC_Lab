#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>


int performanceTest();
void verifyResult(int *seqResult, int *parResult, int n);
void chunckInto(int *chunk,int *cStart,int *cEnd,int n, int tid, int thread_num);
void setup(int **O, int **S, int **R, int **SS, int **RR, int n);
void printDataset(char * description, int * dataset, int size);
void* linkRank(void* a);
void listRankSeq(int *S, int *R, int n);

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
int Ns[NSIZE] = {4096, 8192, 16384, 32768, 65536, 131072, 262144};
//int Ns[NSIZE] = {8};

typedef struct __ThreadArg {
  int id;
  int nrT;
  int n,m;
  int *O, *S, *R, *SS, *RR;
} tThreadArg;


pthread_t callThd[NUM_THREADS];
pthread_mutex_t mutexpm;
pthread_barrier_t barr, internal_barr;



int main (int argc, char *argv[])
{
    performanceTest(1);
}

int performanceTest()
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
        int *O, *S, *R, *SS, *RR;
        setup(&O, &S, &R, &SS, &RR, n);

		printf("| %d | %d |",n,TIMES);

		/* Run sequential algorithm */
		result.tv_usec=0;
		gettimeofday (&startt, NULL);

		for (t=0; t<TIMES; t++) {
            listRankSeq(S, R, n);
		}

		gettimeofday (&endt, NULL);
		result.tv_usec = (endt.tv_sec*1000000+endt.tv_usec) - (startt.tv_sec*1000000+startt.tv_usec);
		printf(" %ld.%06ld | ", result.tv_usec/1000000, result.tv_usec%1000000);

        int seqResult[n];
        for(j=0;j<n;j++)
            seqResult[j] = R[j];

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
				x[j].O = O;
				x[j].S = S;
				x[j].R = R;
				x[j].SS = SS;
				x[j].RR = RR;
                pthread_create(&callThd[j-1], &attr, linkRank, (void *)&x[j]);
			}

			gettimeofday (&startt, NULL);

			pthread_barrier_wait(&barr);
			gettimeofday (&endt, NULL);

			//printDataset("\nS: ", S, n);


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
			printf(" %ld.%06ld | ", result.tv_usec/1000000, result.tv_usec%1000000);
		}
        int parResult[n];
        for(j=0;j<n;j++)
            parResult[j] = R[j];
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


void setup(int **O, int **S, int **R, int **SS, int **RR, int n)
{
    int i,j;

    *O = (int *) malloc(n*sizeof(int));
    *S = (int *) malloc(n*sizeof(int));
    *R = (int *) malloc(n*sizeof(int));
    *SS = (int *) malloc(n*sizeof(int));
    *RR = (int *) malloc(n*sizeof(int));

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

    for(i=0;i<n;i++)
    {
        (*O)[i]=(*S)[i];
    }

}

void printDataset(char * description, int * dataset, int size)
{
    int i;
    printf(description);
    for(i=0; i<size; i++)
        printf("%2i ", dataset[i]);
    printf("\\\n");
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


void* linkRank(void* a){
	/* The code for threaded computation */
    // Perform operations on B
    int t;
    int i,j;
    for (t=0; t<TIMES; t++)
    {
        tThreadArg *tArg = (tThreadArg*) a;
        int *S = tArg->S;
        int *R = tArg->R;
        int *SS = tArg->SS;
        int *RR = tArg->RR;
        int *O = tArg->O;
        int tid = tArg->id;
        int n = tArg->n;
        int thread_num = tArg->nrT;

        int chunk, cStart, cEnd;

        chunckInto(&chunk, &cStart, &cEnd, n, tid, thread_num);

        for(i=cStart;i<cEnd;i++)
        {
            S[i]=O[i];
            SS[i]=S[i];
        }
        pthread_barrier_wait(&internal_barr);

        //printDataset("S!: ", O, n);

        for(i=cStart;i<cEnd;i++)
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
        pthread_barrier_wait(&internal_barr);


        for(j=0;j<(log (n) / log (2));j++)
        {

            for(i=cStart;i<cEnd;i++)
            {
                if(S[i] != S[S[i]])
                {
                    RR[i] = R[i] + R[S[i]];
                    SS[i] = S[S[i]];
                }
            }
            pthread_barrier_wait(&internal_barr);
            for(i=cStart;i<cEnd;i++)
            {
                R[i] = RR[i];
                S[i] = SS[i];
            }
            pthread_barrier_wait(&internal_barr);
        }
    }

    pthread_barrier_wait(&barr);
}

