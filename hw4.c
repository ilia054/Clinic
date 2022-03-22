#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>

#define N 7
#define MAX_TIME 5

sem_t *clinicmutex,*sitmutex;		//mutex for diffrent critical code
sem_t sem[N+2],finished_treatment[N+2],sem1[N+2],paymentmutex[N+2],treatment_sem[N+2]; // patient sem
sem_t couch_full,max_treatment,can_sleep,clinicfull;
sem_t update;
sem_t paymutex;

void Error(char *msg) { printf("%s",msg); exit(1); }

struct Patient {				//linked list for waiting patients
	int id;
	struct Patient *next;
}Patient;

typedef struct Patient P;

P *couch_head = NULL,*stand_head = NULL; 		//couch represents patients that sit on the couch waiting for treatment
												// stand represents patients that stand waiting to sit
												
void addToList(P** head,int id){		//add to tail

	P* node,*tmp;		
	node = (P*)malloc(sizeof(Patient));
	if(node == NULL)
			Error("Memory Allocation Failed!!\n");
	node->id = id;
	node->next = NULL;
	if(*head == NULL){
		*head = node;
		return;
	}
	tmp = *head;
	while(tmp->next != NULL)
		tmp = tmp->next;	
	tmp->next = node;
}

void deleteHead(P** head){			//remove head from list,update new head
	P* tmp = *head;
	*head = tmp->next;
	free(tmp);
}																							   		

void enter(int id){		//function to let patients enter clinic, returns 1 if enters successfully otherwise 0.
	sem_wait(clinicmutex); //enter critical code
	int value;
	sem_getvalue(&clinicfull,&value);
	if(value == 0) //check if clinic is full
		printf("i'm Patient #%d,i'm out of clinic\n",id+1); //no place in the clinic patient is out
	sem_post(clinicmutex); //exit critical code
	sem_wait(&clinicfull); //wait if clinic is full
	addToList(&stand_head,id); //add node to tail
	sem_post(clinicmutex); 	// exit critical code
	printf("i'm Patient #%d,i got into the clinic\n",id+1);
	sleep(1);
}

void sit(int id){
	sem_wait(clinicmutex);   //enter critical code 
	if(stand_head->id != id ){ //there is a patient in queue before you, wait!
		sem_post(clinicmutex); 	// release critical code lock while waiting
		sem_wait(&sem[id]);		//wait until it is your turn to sit on the couch
	}
	else sem_post(clinicmutex); // exit critical code
	sem_wait(&couch_full); //wait until couch has an empty place
	//there is room on the couch
	sem_wait(sitmutex);		//enter critical code 
	addToList(&couch_head,id);  	//add to couch list
	deleteHead(&stand_head);  //remove from standing list
	printf("i'm Patient #%d,i'm sitting on the sofa\n",id+1);
	if(stand_head != NULL){
			sem_post(&sem[stand_head->id]);		//notify next patient he can sit 
	}
	sem_post(sitmutex); //exit sit critical code
	sleep(1);	
}

void getTreatment(int id){  	//patient getting a treatment
	printf("i'm Patient #%d, I'm getting treatment\n",id+1);
	sem_wait(sitmutex); 		//enter critical code
	sem_post(&can_sleep);		//notify dentist there is a patient ready for treatment
	sem_wait(&update);			//wait for dentist to save next patient id
	deleteHead(&couch_head);   //remove patient from couch list
	if(couch_head != NULL)
		sem_post(&treatment_sem[couch_head->id]); //notify next patient he can get treatment
	sem_post(sitmutex);			//exit critical code
	sleep(1);
	if(stand_head != NULL)	 //notify head of standing list he can sit
		sem_post(&sem[stand_head->id]);
	sem_post(&couch_full); 	//notify head of standing list if he is already waiting because couch is full
	sem_post(&sem1[id]);  //inform dentist your ready for treatment
	sem_wait(&finished_treatment[id]);  //wait until treatment is over
}

int giveTreatment(int id){		//dentist giving a treatment
	int patient_id = couch_head->id; 
	sem_post(&update); 			//notify patient that dentist saved id
	int time = rand() % MAX_TIME;
	sem_wait(&sem1[patient_id]); //wait until couch head is ready for treatment
	printf("i'm Dental Hygienist #%d, i'm working now on patient %d\n",id+1,patient_id+1);
	sleep(time);  //treatment for random time
	sem_post(&finished_treatment[patient_id]); //inform patient that treatment is over
	return patient_id;
}

void pay(int id){
	printf("i'm Patient #%d, I'm paying now\n",id+1);
	sem_post(&paymentmutex[id]);  //notify dentist you payed
	sleep(1);
	sem_wait(&paymentmutex[id]);  //wait for dentist to approve
}

void recivePayment(int id,int pid){
	printf("i'm Dental Hygienist #%d, I'm getting a payment\n",id+1);
}

void* Patients(void *p){//function for patients 
	int id,i;
	id = *(int*)p;
	while(1){
		enter(id);
		sit(id);  
		sem_wait(&max_treatment); //wait until there is a dentist free
		if(id != couch_head->id)  //wait for your turn to enter treatment
				sem_wait(&treatment_sem[id]);
		getTreatment(id); 
		sem_wait(&paymutex);  //only one patient can pay at a time -> critical code
		pay(id);
		sem_post(&paymutex);	//exit critical code
		printf("Patient #%d has left the clinic\n",id+1);
		sem_post(&max_treatment);	//notify theres a free dentist
		sem_post(&clinicfull);		//notify your out of the clinic

		sem_init(&sem[id],0,0);      //initialize all semaphores for this patient before re-entring clinic
		sem_init(&sem1[id],0,0);
		sem_init(&finished_treatment[id],0,0);
		sem_init(&paymentmutex[id],0,0);
		sem_init(&treatment_sem[id],0,0);
		sleep(1);
		
		
	}
	
}

void* Doctors(void *p){	//function for doctors
	int id,patient_id;
	id = *(int*)p;
	while(1)
	{
		sem_wait(&can_sleep); //if there are no patients on the couch -> sleep
		patient_id = giveTreatment(id); 
		sem_wait(&paymentmutex[patient_id]);	//wait until patient pays
		recivePayment(id,patient_id);
		sem_post(&paymentmutex[patient_id]); 	//notify patient payment is recived
	}

}


void main() {
	pthread_t patients_t[N+2],doctors_t[3];
	int id[N+2],i;
	
	for (i = 0; i < N+2; i++){ //create patients threads
		id[i] = i;
        if( (pthread_create(&patients_t[i], NULL, Patients, (void*)&id[i])) != 0 )
			Error("Error by creating thread\n");
	}
	
	for (i = 0; i < 3; i++){ //create doc threads
		id[i] = i;
        if( (pthread_create(&doctors_t[i], NULL, Doctors, (void*)&id[i])) != 0 )
			Error("Error by creating Thread\n");
	}
	
	clinicmutex = (sem_t*)malloc(sizeof(sem_t));
	sitmutex = (sem_t*)malloc(sizeof(sem_t));
	if(clinicmutex == NULL || sitmutex == NULL)
		Error("Error in memory allocation\n");
	if (sem_init(clinicmutex, 0, 1) != 0)  // initialize mutex
		Error("Error by creating semaphore\n");
	if (sem_init(sitmutex, 0, 1) != 0)  // initialize mutex
		Error("Error by creating semaphore\n");

		//all semaphore arrays are for patients in diffrent stages in the clinic
	for(i=0;i<N+2;i++)
		sem_init(&sem[i],0,0);
	for(i=0;i<N+2;i++)
		sem_init(&sem1[i],0,0);
	for(i=0;i<N+2;i++)
		sem_init(&finished_treatment[i],0,0);
	for(i=0;i<N+2;i++)
		sem_init(&paymentmutex[i],0,0);
	for(i=0;i<N+2;i++)
		sem_init(&treatment_sem[i],0,0);

	sem_init(&couch_full,0,4);	//sempahore for num of patient on couch
	sem_init(&clinicfull,0,N); //sempahore for num of patients in clinic
	sem_init(&paymutex,0,1); //mutex for paying
	sem_init(&update,0,0);   //semaphore to update patient his id is saved by dentist
	sem_init(&max_treatment,0,3); //semaphore for num of patients getting treatment
	sem_init(&can_sleep,0,0);  //semaphore to make dentist sleep when no patient is waiting for treatment

	for(i=0; i < N+2;i++){
		pthread_join(patients_t[i],NULL);
	}
	for(i=0; i < 3;i++){
		pthread_join(doctors_t[i],NULL);
	}
}






