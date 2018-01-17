#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#define SHSIZE 111111
int * shmarr=NULL;
int temp[111111];
void merge (int l,int m,int r)
{
	int mid = m+1;
	int k=1;
	int safe = l;
	while(l<=m && mid<=r)
	{
		if(shmarr[l] <= shmarr[mid])
			temp[k++] = shmarr[l++];
		else
			temp[k++] = shmarr[mid++];
	}
	while(l<=m)
		temp[k++] = shmarr[l++];
	while(mid <=r)
		temp[k++] = shmarr[mid++];
	for(int i=1;i<k;i++)
		shmarr[safe+i-1] = temp[i];
	return;
}

void mergesort(int l,int r)
{
	if( l < r)
	{
		int n = r-l+1;
		if(n <= 5)
		{
			for(int i=l;i<r;i++)
			{
				int minind = i;
				for(int j=i+1;j<=r;j++)
				{
					if(shmarr[j] < shmarr[minind])
						minind = j;
				}
				int temp = shmarr[minind];
				shmarr[minind] = shmarr[i];
				shmarr[i] = temp;
			}
			return;
		}

		int m = (l+r) >> 1;
		int left_child = fork();
		int right_child;
		int status;
		if( left_child < 0)
		{
			perror("fork");
			exit(1);
		}
		else if(left_child == 0)
		{
			mergesort(l,m);
			exit(0);
		}
		else
		{
			right_child = fork();
			if(right_child < 0)
			{
				perror("fork");
				exit(1);
			}
			else if(right_child == 0)
			{
				mergesort(m+1,r);
				exit(0);
			}
		}
		waitpid(left_child, &status, 0);
		waitpid(right_child, &status, 0);
		merge(l,m,r);
	}
	return;
}

int main()
{
	int shmid;
	key_t key;
	key = 9876;
	int n;
	//printf("No of elements to be sorted:- \n");
	scanf("%d",&n);
	//creates shared memory
	shmid = shmget(key, sizeof(int) * (n+1), IPC_CREAT | 0666);
	if(shmid < 0) //if error
	{
		perror("shmget");
		exit(1);
	}
	//void *shmat(int shmid, const void *shmaddr, int shmflg);	
	// shmat() attaches the System V shared memory segment identified by shmid to the address space of  the  calling  process.
	//  If shmaddr is NULL, the system chooses a suitable  (unused) address at which to attach the segment.
	shmarr = shmat(shmid, NULL , 0);
	// On success, shmat() returns the address of the attached shared memory  segment;
	// on error, (void *) -1 is returned, and errno

	if(shmarr == (void *) - 1)
	{
		perror("shmat");
		exit(1);
	}
	//printf("Enter the elements to be sorted:- \n");
	for(int i=0;i<n;i++)
		scanf("%d",&shmarr[i]);
	mergesort(0,n-1);
	//printf("Sorted array:- \n");
	for(int i=0;i<n;i++)
	{
		printf("%d ",shmarr[i]);
		if(i == n-1)
			printf("\n");
	}
	//int shmdt(const void *shmaddr);
	//shmdt()  detaches  the  shared  memory  segment located at the  address specified by shmaddr
	//from the  address  space  of  the calling process.
	// On success, shmdt() returns 0; on error -1  is  returned,
	if(shmdt(shmarr) == -1)
	{
		perror("shmdt");
		exit(1);
	}

	// int shmctl(int shmid, int cmd, struct shmid_ds *buf);
	// shmctl() performs the control operation specified by cmd on the System V shared memory segment whose identifier is given in shmid.

	//IPC_RMID  Mark  the  segment to be destroyed.  The segment will actually be destroyed only after the last process detaches it
	if(shmctl(shmid,IPC_RMID,NULL) == -1)
	{
		perror("shmctl");
		exit(1);
	}
	return 0;
}
