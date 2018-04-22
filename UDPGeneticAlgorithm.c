#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "data.h"

int fitness[POP_SIZE]; //Keeps track of the fitness of each chromosome
int port; //Used in case a port goes down
int portLoops;

void fprintScheme(FILE* log, Scheme* sptr);
void printScheme(Scheme* sptr); //Prints one scheme out
void initializePopulation(Scheme* population); //Sets up the population with random starting chromosomes
void evaluateFitness(Scheme* population); //Returns the fitness for the chromosome at position chrom
int evaluateScheme(void* arg); //Used to connect to server and evaluate packet loss
void* sendDupes(void* arg); //Sends the duplicates
int* selectParents(int* fitness); //Returns the positions of parents selected by the function
int shouldDo(double percentYes); //Returns 1 if it falls into the percentYes chance, 0 otherwise
void performCrossover(Scheme* chd1, Scheme* chd2); //Performs random point crossover on the two schemes
void performMutation(Scheme* chd); //Performs mutation on the child on one random course
void copyScheme(Scheme* dest, Scheme* src);

int main() {
	srand(time(NULL));

	int i, j;
	int *p1; //The first half of the parents selected
	int *p2; //The second half of the parents selected
	int bestGen = 0; //The generation where the best chromosome occurred
	int bestFit = INT_MIN; //Absolute best fitness found so far
	int minFit, maxFit, avgFit; //Keeps track of fitness per generation
	Scheme bestScheme; //The best Scheme found so far
	Scheme children[POP_SIZE]; //An array that holds the children created from recombination
	Scheme population[POP_SIZE]; //The array of the population
	FILE* logFile;
	
	port = 0;
	portLoops = 0;
	
	initializePopulation(population);
	for(i = 1; i <= MAX_GEN; i++) {
	
		//Resets the min, max and avg fit for each generation
		minFit = INT_MAX;
		maxFit = INT_MIN;
		avgFit = 0;
		
		//Evaluation of fitness for each chromosome
		evaluateFitness(population);
		for(j = 0; j < POP_SIZE; j++) {
			//Checks for best and worst for each generation, then sets up the average
			if(fitness[j] < minFit)
				minFit = fitness[j];
			if(fitness[j] > maxFit)
				maxFit = fitness[j];
			avgFit += fitness[j];
				
			//Sets the new best overall fitness to beat
			if(fitness[j] > bestFit) {
				bestFit = fitness[j];
				bestGen = i;
				
				copyScheme(&bestScheme, &population[j]);
			}
		}
		avgFit /= POP_SIZE;
		//Prints stuff out every so many generations
		if((i % 1) == 0) {
		
			logFile = fopen("logFile.txt", "a");
			if(logFile == NULL) {
				printf("Error opening file.\n");
				return 0;
			}
		
			fprintf(logFile, "Generation %i\n", i);
			fprintf(logFile, "minFit: %i\nmaxFit: %i\navgFit: %i\n\n", minFit, maxFit, avgFit);		
		
			for(j = 0; j < POP_SIZE; j++) {
				fprintf(logFile, "Chrom: %i\nFitness: %i\n", j, fitness[j]);
				fprintScheme(logFile, &population[j]);
				fprintf(logFile, "\n");
				
			}
			fprintf(logFile,"\n\n");
			close(logFile);
		
			//Prints the fitness stats for the current generation
			printf("Generation %i\n", i);
			printf("minFit: %i\nmaxFit: %i\navgFit: %i\n\n", minFit, maxFit, avgFit);
			
			
			//Prints out every scheme in the population along with it's fitness
			/*
			printf("\nGeneration %i:\n\n", i);
			for(j = 0; j < POP_SIZE; j++) {
				printf("Fitness for following scheme: %i\n", fitness[j]);
				printScheme(&population[j]);
				printf("\n\n");
			}*/
		}
		
		//Selection//
		//Creates two arrays from a tournament style selection. The
		//array values are the position of the parents. Each selection process will
		//only select a parent once, so at most, a parent can be chosen twice.
		//Memory is dynamically allocated in the function, so must be freed.
		
		//First set of parents to be
		p1 = selectParents(fitness);
		if(p1 == NULL) {
			printf("An error occured when allocating memory\n");
			return 0;
		}
		//Second set of parents to be
		p2 = selectParents(fitness);
		if(p2 == NULL) {
			printf("An error occured when allocating memory\n");
			return 0;
		}
		
		//Recombination//
		//Copies the parents into the children array, 
		//then may perform crossover and mutation
		for(j = 0; j < POP_SIZE/2; j++) {
			
			//Copies each parent into the children array
			copyScheme(&children[j*2], &population[p1[j]]);
			copyScheme(&children[j*2 +1], &population[p2[j]]);			
			
			//May perform crossover. If it does, the two children passed in
			//are crossedover at a random point.
			if(shouldDo(pc))
				performCrossover(&children[j*2], &children[j*2 +1]);
				
			//Possible mutation. Mutation changes one random courses room and time
			if(shouldDo(pm))
				performMutation(&children[j*2]);
				
			if(shouldDo(pm))
				performMutation(&children[j*2 +1]);
		}
		

		//Copies the children to make them the new parents
		for(j = 0; j < POP_SIZE; j++)
			copyScheme(&population[j], &children[j]);
		
		//Frees up the parent arrays and resets them to NULL
		free(p1);
		p1 = NULL;
		free(p2);
		p2 = NULL;

		
	}

	logFile = fopen("logFile.txt", "a");
	if(logFile == NULL) {
		printf("Error opening file.\n");
		return 0;
	}

	fprintf(logFile, "Best fitness: %i\n", bestFit);
	fprintf(logFile, "Found in generation %i\n", bestGen);
	fprintScheme(logFile, &bestScheme);

	close(logFile);

	//Prints the fitness of the best scheme found through all the generation,
	//what generation it was found, and the actual scheme. Prints the first occurance
	//of this fitness score.
	printf("Best fitness: %i\n", bestFit);
	printf("Found in generation %i\n", bestGen);
	printScheme(&bestScheme);
	
	return 0;
	
}

void fprintScheme(FILE* log, Scheme* sptr) {
	fprintf(log, "Size: %i\n", DATA_SIZES[sptr->size]);
	fprintf(log, "Order: %i\n", sptr->order);
	fprintf(log, "delay: %i\n", sptr->delayTilDuplicate);
}

void printScheme(Scheme* sptr) {
	//Do stuff
	printf("Size: %i\n", DATA_SIZES[sptr->size]);
	printf("Order: %i\n", sptr->order);
	printf("delay: %i\n", sptr->delayTilDuplicate);
}

//Randomly generates a set of chromosomes
void initializePopulation(Scheme* population) {
	int i;
	
	
	for(i = 0; i < POP_SIZE; i++) {
		population[i].size = rand() % NUM_OF_SIZES;
		population[i].order = rand() % NUM_OF_ORDERS;
		population[i].delayTilDuplicate = rand() % 99 + 1;
	}
}

//Creates a thread to evaluate fitness
void evaluateFitness(Scheme* population) {
	
	int i, j;
	int result;
	
	pthread_t myThread[POP_SIZE];
	TestScheme_t* data;
	
	for(i = 0; i < POP_SIZE; i++) {
		data = malloc(sizeof(TestScheme_t));
		data->schemeNum = i;
		data->portToUse = port;
		copyScheme(&(data->scheme), &population[i]);
		result = evaluateScheme((void*)data);
		sleep(1);
		if(result == 0) {
			sleep(5);
			i--;
			port++;
			//printf("Current port: %i\n". (FIRST_PORT + port));
			if(port >= 60) {
				if(portLoops < 10) {
					port = 0;
					portLoops++;
				} else {
					return;
				}
			}
		}
	}
	
	/*
	for(j = 0; j < POP_SIZE/numOfThreadsAtOnce; j++) {
		for(i = 0; i < numOfThreadsAtOnce; i++) {
			
			data = malloc(sizeof(TestScheme_t));
			data->schemeNum = (numOfThreadsAtOnce * j) + i;
			data->portToUse = i;
			copyScheme(&(data->scheme), &population[(numOfThreadsAtOnce * j) + i]);
			pthread_create(&myThread[(numOfThreadsAtOnce * j) + i], NULL, evaluateScheme, (void*) data);
			usleep(200); //So the threads aren't all hitting the server at once
		}
		//Doesn't continue until every thread is complete
		for(i = 0; i < numOfThreadsAtOnce; i++) {
			pthread_join(myThread[(numOfThreadsAtOnce * j) + i], NULL); 
		}
	}
	*/
}

//Where all the networking magic happens
int evaluateScheme(void* arg) {
	TestScheme_t* data = (TestScheme_t*)arg;
	//fitness[data->schemeNum] = 0;
	//fitness[data->schemeNum] += data->scheme.size + data->scheme.order + data->scheme.delayTilDuplicate;
	struct sockaddr_in addrUDP, addrTCP;
	int sockfdUDP, sockfdTCP, lenUDP, lenTCP, ret, i;
	FILE* wavfd;
	int msgSize;
	int packetsRecv;
	char buf[50];
	char* msgToSend;
	pthread_t dupeThread;
	struct timeval tv;
	fd_set my_fds;
	//char* msgRecv;
	
	//Setting up of the socket for UDP and TCP with the address and port.
	sockfdUDP = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfdUDP == -1) {
		perror(strerror(errno));
		printf("Fail on UDPSocket\n");
		free(data);
		return 0;
	}
	
	sockfdTCP = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfdTCP == -1) {
		perror(strerror(errno));
		close(sockfdTCP);
		printf("Fail on TCP Socket.\n");
		free(data);
		return 0;
	}
	
	addrUDP.sin_family = AF_INET;
	addrUDP.sin_addr.s_addr = inet_addr(AWS_ADDR);	
	addrUDP.sin_port = htons(FIRST_PORT + data->portToUse);
	lenUDP = sizeof(addrUDP);

	addrTCP.sin_family = AF_INET;
	addrTCP.sin_addr.s_addr = inet_addr(AWS_ADDR);	
	addrTCP.sin_port = htons(FIRST_PORT + data->portToUse);
	lenTCP = sizeof(addrTCP);
	
	
	ret = connect(sockfdTCP, (struct sockaddr*)&addrTCP, lenTCP);
	if(ret == -1) {
		perror(strerror(errno));
		close(sockfdUDP);
		close(sockfdTCP);
		printf("Fail on connect.\n");
		free(data);
		return 0;
	}
	
	//First send the size each packet is going to be
	sprintf(buf, "%i", DATA_SIZES[data->scheme.size]);
	write(sockfdTCP, buf, strlen(buf) + 1);
	usleep(1000); //Sleeps 1ms to give the server time to prepare to get packets
	

	
	if(data->scheme.order != 0) {
		dupeData_t dupeData;
		copyScheme(&dupeData.scheme, &(data->scheme));
		dupeData.sockfd = sockfdUDP;
		dupeData.addr = addrUDP;
		dupeData.len = lenUDP;
		pthread_create(&dupeThread, NULL, sendDupes, (void*) &dupeData);	
	}
	
	msgToSend = malloc((5+DATA_SIZES[data->scheme.size])*sizeof(char));
	wavfd = fopen(NAME_OF_FILE, "rb");
	if(wavfd == 0) {
		printf("Error opening file.\n");
		close(sockfdUDP);
		close(sockfdTCP);
		free(data);	
		return 0;
	}
	//Now loops until all packets are sent
	for(i =0; i < SIZE_OF_FILE/DATA_SIZES[data->scheme.size]; i++) {
		sprintf(msgToSend, "%04d%c", i, '\0');
		fread(&msgToSend[5], 1, DATA_SIZES[data->scheme.size], wavfd);
		sendto(sockfdUDP, msgToSend, 5+DATA_SIZES[data->scheme.size], 0 ,(struct sockaddr*)&addrUDP, lenUDP);
		usleep(TIME_FOR_SIZES[data->scheme.size] * 1000);
	}
	if(data->scheme.order != 0)
		pthread_join(dupeThread, NULL);
	free(msgToSend);
	
	tv.tv_sec = 30;
	tv.tv_usec = 0;
	FD_ZERO(&my_fds);
			
	FD_SET(sockfdTCP, &my_fds);
	tv.tv_sec = 90;
	tv.tv_usec = 0;
	ret = select(sockfdTCP + 1, &my_fds, NULL, NULL, &tv);
	if(ret == -1)//select fail
	{
		perror("on select");
		close(sockfdUDP);
		close(sockfdTCP);
		close(wavfd);
		free(data);		
		return 0; 
	}
	else if(ret == 0) //timeout
	{
		printf("Timeout on TCP read.\n");
		close(sockfdUDP);
		close(sockfdTCP);
		close(wavfd);
		free(data);
		return 0;
	}
	else
	{
		read(sockfdTCP, buf, sizeof(buf)); //Reads the numbers of packets lost
	}
	
	packetsRecv = atoi(buf);
	
	//Should set the fitness between 0 and 100, the number being the percent of packets successfully received
	fitness[data->schemeNum] = (((double)packetsRecv * (double)DATA_SIZES[data->scheme.size])/(double)SIZE_OF_FILE) * 100; 
	printf("fitness[%i]: %i\n", data->schemeNum, fitness[data->schemeNum]);
	printf("pRecv: %i   size: %i\n", packetsRecv, DATA_SIZES[data->scheme.size]);
	
	close(sockfdUDP);
	close(sockfdTCP);
	close(wavfd);
	free(data);
	return 1;
}

void* sendDupes(void* arg) {
	FILE* wavfd;
	int initDelay;
	int i;
	dupeData_t* data = (dupeData_t*) arg;
	initDelay = data->scheme.order - 1;
	char* msgToSend;
	msgToSend = malloc((5+DATA_SIZES[data->scheme.size])*sizeof(char));
	wavfd = fopen(NAME_OF_FILE, "rb");
	if(wavfd == 0) {
		printf("Error opening file.\n");	
		return 0;
	}
	
	usleep((TIME_FOR_SIZES[data->scheme.size] * 1000) * initDelay); //Sleeps depending on order
	usleep((int)(((double)data->scheme.delayTilDuplicate/100.0) * (TIME_FOR_SIZES[data->scheme.size] * 1000)));//Sleep depending on the offset
	//Now loops until all packets are sent
	for(i =0; i < SIZE_OF_FILE/DATA_SIZES[data->scheme.size]; i++) {
		sprintf(msgToSend, "%04d%c", i, '\0');
		fread(&msgToSend[5], 1, DATA_SIZES[data->scheme.size], wavfd);
		sendto(data->sockfd, msgToSend, 5+DATA_SIZES[data->scheme.size], 0 ,(struct sockaddr*)&(data->addr), data->len);
		usleep(TIME_FOR_SIZES[data->scheme.size] * 1000);
	}	
	close(wavfd);
}


//ONLY WORKS FOR EVEN NUMBER POPULATIONS
//Does a tournament selection of parents
//Creates an array of numbers from 0 to to POP_SIZE - 1
//Randomly picks through that array to make sure each contestent is only selected
//once. Then the one with the highest fitness is added to the parents to be array.
//Returns an array of integers. The integers correspond to index of the parent to use.
//There will be POP_SIZE/2 parents.
//*****MEMORY MUST BE FREED ONCE PASSED BACK*****//
int* selectParents(int* fitness) {

	int *parents; //The array to be returned
	int *numArr; //Array of integers from 0 to the POP_SIZE
	int i, numLeft, n;
	int cont1, cont2; //The two contestants to compare
	
	parents = malloc((POP_SIZE/2) * sizeof(int));
	if (parents == NULL)
		return NULL;

    numArr = malloc((POP_SIZE )*sizeof(int));
	if (numArr == NULL)
		return NULL;
		
    for (i = 0; i  < POP_SIZE; i++)
        numArr[i] = i;
    numLeft = POP_SIZE;
	
	for(i = 0; i <POP_SIZE/2; i++) {
		n = rand() % numLeft;
		cont1 = numArr[n];
		numArr[n] = numArr[numLeft - 1];
		numLeft--;
		
		n = rand() % numLeft;
		cont2 = numArr[n];
		numArr[n] = numArr[numLeft - 1];
		numLeft--;
		
		if(fitness[cont1] >= fitness[cont2]) {
			parents[i] = cont1;
		} else {
			parents[i] = cont2;
		}
	}
	
	free(numArr);
	return parents;
}
//Returns 1 percentYes percent of the time(1 = 100%, .5 = 50%, etc.)
//Returns 0 otherwise
int shouldDo(double percentYes) {
	int yes = (int)(percentYes * 1000);
	if((rand() % 1000) < yes)
		return 1;
		
	return 0;
}

//Random point crossover.
void performCrossover(Scheme* chd1, Scheme* chd2){
	int crspt = rand() % 2;//Number of chromosomes
	
	Scheme temp;
	copyScheme(&temp, chd1);
	if(crspt == 0) {
		chd1->size = chd2->size;
		chd2->size = temp.size;
	} else {
		chd1->size = chd2->size;
		chd2->size = temp.size;
		chd1->order = chd2->order;
		chd2->order = temp.order;
	}

}

//Mutates a single gene in the chromosome
void performMutation(Scheme* chd){ 
	int mutpt = rand() % 3;
	
	if(mutpt == 0) {
		//Size being mutated
		chd->size = rand() % NUM_OF_SIZES;
	} else if (mutpt == 1) {
		//Scheme being mutated
		chd->order = rand() % NUM_OF_ORDERS;
	} else {
		//Delay being mutated
		chd->delayTilDuplicate = rand() % 99 + 1;
	}
}


void copyScheme(Scheme* dest, Scheme* src) {
	dest->size = src->size;
	dest->order = src->order;
	dest->delayTilDuplicate = src->delayTilDuplicate;
}

