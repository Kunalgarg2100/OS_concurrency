#include<stdio.h>
#include<stdlib.h>
#include<semaphore.h>
#include<pthread.h>
#include<unistd.h>

#define TOTALCARS  10
#define TOTALATTENDERS  3
#define MAXCAPACITY  7
#define TOTALGASPUMPS  3
#define MAXINQUEUE  4
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"


sem_t maxCapacity;

sem_t mutex_gasstation;

sem_t mutex_queue;

sem_t totalGaspumps;

sem_t carReady; // used when car is ready for service or to give payment

sem_t paymentCompletion; // used to indicate that payment process has been completed

int pipe1[2];  //pipe between car and  attendee
int pipe2[2];  //pipe between car and cashcounter 

sem_t mutex_pipe1; //mutex_pipe1 is used to lock and unlock during writing and reading between car and attendee

sem_t mutex_pipe2;  //mutex_pipe1 is used to lock and unlock during writing and reading between car and cashcounter

sem_t readywithpayment; //mutex to lock and unlock during for payment between car and cashier(attendee)

int stations_available = TOTALGASPUMPS; //gasstations currently free
int spaceInqueue = MAXINQUEUE; // how many more cars can join queue
int gasStation[TOTALGASPUMPS]; 
sem_t leavepump[TOTALGASPUMPS];

sem_t finished[TOTALCARS]; 
sem_t done[TOTALCARS];
sem_t receipt[TOTALCARS];
pthread_t threads[70];

void read_pipe(int readfd,int * value)
{
	read(readfd,value,sizeof(int));
}

void write_pipe(int writefd,int value)
{
	write(writefd,&value,sizeof(int));
}

void enterStation(int CarId)
{
	/* Waiting ouside the gas station to get in queue */
	sem_wait(&maxCapacity);
	printf(ANSI_COLOR_CYAN "Car %d enters station\n" ANSI_COLOR_RESET,CarId);
}


void waitInLine(int CarId)
{
	sem_wait(&mutex_queue);
		spaceInqueue--;
		sem_post(&mutex_queue);

		printf("Car %d waits in Queue\n",CarId);
		sem_wait(&totalGaspumps); //waits for empty station while car is in queue
		sem_wait(&mutex_queue);
		spaceInqueue++; //leaves the queue
		sem_post(&mutex_queue); //gives signal of leaving queue(increments the space in queue)
		printf("Car %d proceeds to gas station\n",CarId);
}

void pay(int CarId)
{
	sem_wait(&mutex_pipe2); //lock for writing in pipe2
	write_pipe(pipe2[1],CarId); //Pipe2 stores the carid which has to pay
	printf("Car %d ready to pay\n",CarId); //Car is ready to pay 
	sem_post(&mutex_pipe2); //Lock is realised
	sem_post(&readywithpayment);//Ready with payment signalled to cashCounter()
}

int goToPump(int CarId)
{
	/* Secure gasstation selection and changing the value of stations_avialable */
	sem_wait(&mutex_gasstation);
	int i = 0; 
	while ((i < TOTALGASPUMPS) && (gasStation[i] != -1) ) //search for empty gasStation
		i++;
	stations_available--; 
	gasStation[i] = CarId;
	printf("Car %d occupies gasStation %d. Stations Available = %d\n",CarId,i,stations_available);
	sem_post(&mutex_gasstation);
	/* Putting Car Id and gasStation's Id in a pipe for the attendee
	   and sending signal to attendee that car is ready for servicing */
	sem_wait(&mutex_pipe1);
	write_pipe(pipe1[1],CarId); 
	write_pipe(pipe1[1],i);
	sem_post(&mutex_pipe1);
	sem_post(&carReady);
	return i;
}

void exitStation(int CarId)
{
	sem_wait(&receipt[CarId]); //Waits for receipt to be granted
	printf(ANSI_COLOR_RED "Car %d leaving station\n" ANSI_COLOR_RESET,CarId); // After receipt is granted Car leaves the Station
	sem_post(&done[CarId]); //done signal for Car which left the station
	sem_post(&maxCapacity); //increments max capacity

	if (CarId == (TOTALCARS-1)) //If this is the last car  
	{
		for(int k=0;k <= (TOTALCARS-1); k++) 
			sem_wait(&done[k]);	//wait this all cars are serviced
		printf("All cars are served\n");
		exit(0); //If the all are served exit the process
	}

	else //All cars except the last car exits the thread
		pthread_exit(0);
}

void leavePump(int CarId,int i)
{
	/* wait for sem_post from attendee that servicing is done */
	sem_wait(&finished[CarId]); 
	/* sem_post the right gasStation to increment stations_available securely*/
	sem_wait(&mutex_gasstation); 
	gasStation[i] = -1;
	stations_available++;
	sem_post(&mutex_gasstation);
	sem_post(&leavepump[i]);
	printf("Car %d left gasStation %d. Stations Available = %d\n",CarId,i,stations_available); 

}

void Car(int CarId) 
{
	enterStation(CarId);
	/* do we go straight to a attendee's gasStation? or we have to wait in queue? check securely */
	sem_wait(&mutex_gasstation);
	sem_wait(&mutex_queue);
	if (stations_available == 0)  //If no station is available
	{
		sem_post(&mutex_queue);
		sem_post(&mutex_gasstation);
		waitInLine(CarId);		
	}  
	else //If empty station is available
	{ 
		sem_post(&mutex_queue); //releases lock of queue(which changes spaceInqueue)
		sem_post(&mutex_gasstation); //releases lock of gasstation(which changes stations_available)
		sem_wait(&totalGaspumps); //waits for signal empty station
	}

	int pumpNum = goToPump(CarId);
	leavePump(CarId,pumpNum);
	pay(CarId);
	exitStation(CarId);
}

void serveCar(int attendeeNum,int carNum ,int stationNum)
{
	/* Serving of car Started */
	printf("Attendee %d started servicing Car %d on gasStation %d\n",attendeeNum,carNum,stationNum);
	/* Delay of one before it sends signal that service of Car is finished */ 
	sleep(1);
	sem_post(&finished[carNum]); //Car service is finished
	printf("Service of Car %d finished by Attendee %d on gasStation %d\n",carNum,attendeeNum,stationNum);

	sem_wait(&leavepump[stationNum]); //waits for car to leavepump
	sem_post(&totalGaspumps); /* now sem_post that a gasStation is free */
	printf("Attendee %d has seen  Car %d leave gasStation %d\n",attendeeNum,carNum,stationNum);
}

void Attendee(int attendeeNum) 
{
	while(1) 
	{
		int carNum, stationNum;
		/* Attendee dreams till there is there a car at gasStation or there is a call from the cash counter  */
		printf("Attendee %d: dreaming about his own Car\n",attendeeNum);
		sem_wait(&carReady);

		/* obtain carId and gasStationId securely and works accordingly*/
		sem_wait(&mutex_pipe1);
		read_pipe(pipe1[0],&carNum);
		read_pipe(pipe1[0],&stationNum);
		sem_post(&mutex_pipe1);

		if (carNum != -1) /* if car is to be served */
			serveCar(attendeeNum,carNum,stationNum);
		else	/* if we have to accept payment */
		{ 
			sleep(0.5);
			printf(ANSI_COLOR_GREEN  "Attendee %d accepts payment of Car %d\n" ANSI_COLOR_RESET,attendeeNum,stationNum);
			sem_post(&paymentCompletion);
		}
	} 
}

void cashcounter() {
	while(1) {
		int Carid;
		sem_wait(&readywithpayment); /* wait for a Car to arrive here with payment */

		/*Detecing which car is here for payment securely*/
		sem_wait(&mutex_pipe2); 
		read_pipe(pipe2[0],&Carid); 
		sem_post(&mutex_pipe2); 
		
		/* printing which car has arrived for payment */
		printf(ANSI_COLOR_GREEN "Cashcounter:" ANSI_COLOR_RESET "Car %d has arrived with payment. Calling a attendee\n",Carid);

		/* Writing to pipe for calling attendence to accept payment securely */
		sem_wait(&mutex_pipe1);
		write_pipe(pipe1[1],-1); /* -1 means that it has been served and is here to pay*/
		write_pipe(pipe1[1],Carid); /* car id is send as station id */
		sem_post(&mutex_pipe1);
		sem_post(&carReady); /*Car is ready to pay */

		/* wait for a sem_post from a attendee that payment has been received */
		sem_wait(&paymentCompletion);
		printf(ANSI_COLOR_GREEN "Cashcounter:" ANSI_COLOR_RESET "Car %d has paid to a attendee\n" ,Carid);

		sem_post(&receipt[Carid]); /* let go the right customer */
	} 
}


/* Fuction in a thread to make car */
void carMaker()
{
	printf("*Car Maker Created*\n\n");
	fflush(stdout);
	/* Function creates a car after a radom time */
	for(int i=0;i<TOTALCARS;i++) 
	{
		sleep(rand()%3);
		pthread_create(&threads[i+TOTALATTENDERS],NULL,(void *)&Car,(void *)i);
		printf("Car created %d\n",i );
	}
	/* After creating total cars loop exists */
	pthread_exit(0);
}

int main() 
{ 
	/* For rand generation of numbers */
	int iseed=time(NULL);
	srand(iseed);

	sem_init(&maxCapacity,0,MAXCAPACITY);  //maxCapacity of the station
	sem_init(&mutex_gasstation,0,1); //mutex_gasstation is used when we are changing the state of any gasstation
	sem_init(&totalGaspumps,0,TOTALGASPUMPS); //total gas pumps in station
	sem_init(&carReady,0,0);
	sem_init(&paymentCompletion,0,0);
	pthread_t carMaker_thread;
	sem_init(&mutex_pipe1,0,1); //mutex_pipe1 is used to lock and unlock during writing and reading from pipe1
	sem_init(&mutex_pipe2,0,1);	//mutex_pipe1 is used to lock and unlock during writing and reading from pipe2
	sem_init(&mutex_queue,0,1);	//mutex_gasstation is used when we are updating the value of cars waiting in queue

	for(int i=0;i<TOTALCARS;i++)
		sem_init(&receipt[i],0,0); //receipt[i] -> ith car has paid or not? iniatiasized to 0
	for(int i=0;i<TOTALCARS;i++)
		sem_init(&finished[i],0,0);  //finished[i] -> service of ith car has been finished or not? iniatiasized to 0

	for(int i=0;i<TOTALGASPUMPS;i++)
		gasStation[i]=-1; //All gasstations are empty initially
	for(int i=0;i<TOTALGASPUMPS;i++)
		sem_init(&leavepump[i],0,0);  

	/* Creating two pipes */
	pipe(pipe1);
	pipe(pipe2);

	printf("Creating Attendees\n");
	int i=1;
	while (i <= TOTALATTENDERS) 
	{
		pthread_create(&threads[i],NULL,(void *)&Attendee,(void *)i);
		printf("Attendee %d created\n",i);
		i++;
	}

	printf("Creating Cars\n");
	pthread_create(&carMaker_thread,NULL,(void *)&carMaker,NULL);

	cashcounter();
	pthread_join(carMaker_thread,NULL);
}
