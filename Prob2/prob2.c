#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

const int NEW_VOTER=0;

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ZERO 0
typedef struct Booth Booth;
typedef struct EVM EVM;
typedef struct Voter Voter;

struct EVM
{
	int idx,flag,number_of_slots;
  pthread_t evm_thread_id;
  Booth * booth;
};

EVM* evmInitialisation(EVM * evm, int idx, Booth * booth)
{
  int temp=0;
  evm = (EVM*)malloc(sizeof(EVM));
  evm->number_of_slots = 0,evm->flag = 0;
  evm->idx = idx,evm->booth = booth;
  temp =1;
  return evm;
}

struct Voter
{
	int idx,status;
	pthread_t voter_thread_id;
	Booth * booth;
	EVM * evm;
};

Voter* voterInitialisation(Voter * voter, int idx, Booth * booth)
{
  int temp=0;
  voter = (Voter*)malloc(sizeof(Voter));
  voter->idx = idx,voter->booth = booth;
  voter->evm = NULL;
  voter->status = NEW_VOTER;
  temp = 1;
  return voter;
}

struct Booth
{
	int idx;
  int numofVoters,done_voters,numofEVMS,max_slots_in_evm;
	pthread_t booth_thread_id;
  Voter ** voters;
	EVM ** evms;
	pthread_mutex_t mutex;
	pthread_cond_t cv_1,cv_2;
};

Booth* boothInitialisation( Booth * booth, int idx, int numofEVMS, int max_slots_in_evm, int numofVoters)
{
  int temp = 0;
  booth = (Booth*)malloc(sizeof(Booth));
  booth->idx = idx;
  booth->numofVoters = numofVoters,booth->done_voters = 0;
  booth->numofEVMS = numofEVMS, booth->max_slots_in_evm = max_slots_in_evm;
  booth->voters = (Voter**)malloc(sizeof(Voter*)*numofVoters);
  pthread_mutex_init(&(booth->mutex), NULL),pthread_cond_init(&(booth->cv_1), NULL), pthread_cond_init(&(booth->cv_2), NULL);
  booth->evms = (EVM**)malloc(sizeof(EVM*)*numofEVMS);
  temp = 1;
  return booth;
}

int WAITINGVOTER=1;
int ASSIGNEDVOTER=2;
int COMPLETEDVOTER=3;

void * voterThread(void * args)
{
  Voter * voter = (Voter*)args;
  pthread_cond_t * cv_1_ptr = &(voter->booth->cv_1), * cv_2_ptr = &(voter->booth->cv_2);
  pthread_mutex_t * mutex_booth = &(voter->booth->mutex);
  int temp = 0;
  pthread_mutex_lock(mutex_booth);
  voter->status = WAITINGVOTER;
  while(voter->status == WAITINGVOTER) 
    pthread_cond_wait(cv_1_ptr, mutex_booth); /* voter_wait_for_evm */
  pthread_mutex_unlock(mutex_booth);
  temp = 1;

  EVM * evm = voter->evm;
  temp = ZERO;
  pthread_mutex_lock(mutex_booth);
  while(evm->flag == ZERO)
    pthread_cond_wait(cv_1_ptr, mutex_booth);
  (evm->number_of_slots)--;/* voter_in_slot */
  printf("Voter %d at Booth %d has casted its vote at EVM %d\n", voter->idx+1,evm->booth->idx+1, evm->idx+1);
  pthread_cond_broadcast(cv_2_ptr);
  pthread_mutex_unlock(mutex_booth);
  temp = 1;
  return NULL;
}

void * evmThread(void * args){
	EVM * evm = (EVM*)args;
  Booth * booth = evm->booth;

  pthread_cond_t * cv_2_ptr = &(booth->cv_2),* cv_1_ptr = &(booth->cv_1);
	pthread_mutex_t * mutex_booth = &(booth->mutex);

	while(2){
		pthread_mutex_lock(mutex_booth);
		if(booth->done_voters == booth->numofVoters)
		{
			pthread_mutex_unlock(mutex_booth);
			break;
		}
		pthread_mutex_unlock(mutex_booth);

		int j = 0;
		int slots_in_evm = rand()%(booth->max_slots_in_evm) + 1;
		pthread_mutex_lock(mutex_booth);
		evm->flag = ZERO;
    evm->number_of_slots = slots_in_evm;
		pthread_mutex_unlock(mutex_booth);
		printf("Evm %d at Booth is %d free with slots = %d\n", evm->idx+1 ,booth->idx+1, slots_in_evm);

		while(1)
		{
      if(j == slots_in_evm)
        break;
			int i = rand() % booth->numofVoters;
      int temp = ZERO;
			pthread_mutex_lock(mutex_booth);
      int k=0;
			if(booth->voters[i]->status == WAITINGVOTER)
			{
        booth->voters[i]->status = ASSIGNEDVOTER;
        booth->voters[i]->evm = evm;
        (booth->done_voters)++;
				printf("Voter %d at Booth %d got allocated with EVM %d\n", i+1,booth->idx+1, evm->idx+1);
        j++;
			}
			if(booth->done_voters == booth->numofVoters)
			{
				pthread_mutex_unlock(mutex_booth);
				break;
			}
			pthread_mutex_unlock(mutex_booth);
      temp = 1;
		}

		if(j==0)
			break;

		/* evm executing voting phase. */
		printf(ANSI_COLOR_CYAN  "Evm %d at Booth %d is moving to voting phase.\n" ANSI_COLOR_RESET,  evm->idx+1,booth->idx+1);

		pthread_mutex_lock(mutex_booth);
		evm->number_of_slots = j;
		evm->flag = 1;
		pthread_cond_broadcast(cv_1_ptr);
		while(evm->number_of_slots)
			pthread_cond_wait(cv_2_ptr, mutex_booth);
		pthread_mutex_unlock(mutex_booth);

		printf(ANSI_COLOR_GREEN "Evm %d at Booth %d has finished voting phase.\n" ANSI_COLOR_RESET,  evm->idx+1,booth->idx+1);
	}
	return NULL;
}

void * booth_thread(void* args)
{
	Booth * booth = (Booth*)args;
  /* evms and voters init */

  int i=0;
  while(1)
  {
    if(i == (booth->numofVoters))
      break;
    booth->voters[i] = voterInitialisation(booth->voters[i], i, booth);i++;
  }
  i=0;
  while(1)
  {
    if(i == (booth->numofEVMS))
      break;
    booth->evms[i] = evmInitialisation(booth->evms[i], i, booth);i++;
  }

  /* voters and evms threads start */
  i=0;
  while(1)
  {
    if(i == (booth->numofVoters))
      break;
    pthread_create(&(booth->voters[i]->voter_thread_id),NULL, voterThread, booth->voters[i]);i++;
  }
  i=0;
  while(1)
  {
    if(i == (booth->numofEVMS))
      break;
    pthread_create(&(booth->evms[i]->evm_thread_id),NULL, evmThread, booth->evms[i]);i++;
  }

	/* evms and voters threads joined */
	for(int i=0,j=0; i<booth->numofEVMS; i++)
		pthread_join(booth->evms[i]->evm_thread_id, 0);

	for(int i=0; i<booth->numofVoters; i++)
		pthread_join(booth->voters[i]->voter_thread_id, 0);

	printf(ANSI_COLOR_YELLOW "Voters at Booth Number %d are done with voting.\n" ANSI_COLOR_RESET, booth->idx+1);

	/* freeing memory */
  for(int i=0; i<booth->numofEVMS; i++)
    free(booth->evms[i]);
  for(int i=0; i<booth->numofVoters; i++)
    free(booth->voters[i]);
  free(booth->evms);
  free(booth->voters);
	return NULL;
}



int main()
{
	int number_of_booths,numEvms,numVoters;
	scanf("%d", &number_of_booths);
  int * max_slots_in_evm = (int*)malloc(sizeof(int)*number_of_booths);

  Booth ** booths = (Booth**)malloc(sizeof(Booth*)*number_of_booths);
	int * numofEVMS = (int*)malloc(sizeof(int)*number_of_booths);
	int * numofVoters = (int*)malloc(sizeof(int)*number_of_booths);

	for(int i=0; i<number_of_booths; i++)
	{
		scanf("%d%d",&numVoters, &numEvms);
    max_slots_in_evm[i] = 10;
		numofVoters[i] = numVoters;
		numofEVMS[i] = numEvms;
	}
	printf("ELECTION STARTED.\n");

	/* booth init */
  int i=0;
  while(1){
    if(i == number_of_booths)
      break;
    booths[i] = boothInitialisation( booths[i], i, numofEVMS[i], max_slots_in_evm[i], numofVoters[i]);i++; }

	/* booth thread start */
	for(int i=0; i<number_of_booths; i++)
		pthread_create(&(booths[i]->booth_thread_id), NULL, booth_thread, booths[i]);

	/* booth thread joined */
	for(int i=0; i<number_of_booths; i++)
		pthread_join(booths[i]->booth_thread_id, 0);
	printf("ELECTION COMPLETED\n");

	/* freeing alloted memory */
	free(numofVoters);
	free(numofEVMS);
	free(max_slots_in_evm);

	for(int i=0; i<number_of_booths; i++)
		free(booths[i]);
	free(booths);

	return 0;
}
