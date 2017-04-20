//IOS 2.projekt
//Adam Bezak xbezak01
//xbezak01@stud.fit.vutbr.cz
//h2o.c

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h> 
#include <sys/types.h> 
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/wait.h> 
#include <semaphore.h> 
#include <unistd.h> 
#include <time.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/mman.h>
#include <string.h>

/*
*	Zdielana struktura
*/
typedef struct shared_var { 
	int line_count; // pocet riadkov
	int oxygen_wait; // pocitadlo kyslikov
	int hydrogen_wait; // pocitadlo vodikov
	char *status; // vypis
	int finished_processes; // pocet skoncenych procesov
	int process_counter; // pocet vsetkych procesov
	int bonded; // pocet procesov v stave bondedd
} shared_var;

/*
*	Konstanty
*/
enum Errors {bad_arg_N, bad_arg_GN, bad_arg_GO, bad_arg_B, bad_arg_count,process_fail,		//nazvy errorov
	memory_fail,exit_fail,file_open_error,file_close_error,sem_crt_fail,
	sem_dst_fail};
const char *ErrCode[] =                                                                 	//ErrCode
{
 [bad_arg_N] = "Pocet procesov reprezentujucich kyslik musi byt vacsi ako 0.\n",
 [bad_arg_GN] = "Doba vytvorenia vodiku musi byt v rozmedzi 0 - 5000 ms.\n",
 [bad_arg_GO] = "Doba vytvorenia kyslik musi byt v rozmedzi 0 - 5000 ms.\n",
 [bad_arg_B] = "Doba priebehu funkcie bond musi byt v rozmedzi 0 - 5000 ms.\n",
 [bad_arg_count] = "Zadali ste nespravny pocet argumentov.\n",
 [process_fail] = "Proces sa nepodarilo vytvorit.\n",
 [memory_fail] = "Nastala chyba pri alokacii zdielanej pamate.\n",
 [exit_fail] = "Nastala chyba pri ukoncovani procesu.\n",
 [file_open_error] = "Subor sa nepodarilo otvorit.\n",
 [file_close_error] = "Subor sa nepodarilo uzaviret.\n",
 [sem_crt_fail] = "Nastala chyba pri vytvarani semaforu.\n",
 [sem_dst_fail] = "Nastala chyba pri ruseni semaforu.\n"
};

/*
*	Globalne premene
*/
int ID_shared_mem;	// sektor v ktorom je alokovana zdielana pamat
struct shared_var *shared_data;
FILE *file;
sem_t *Semaphore_write,*Semaphore_barrier,*Semaphore_bonding
,*Semaphore_shared_mem, *my_hydro_queue, *my_oxygen_queue, *my_mutex, *waiting, *turnstile1, *turnstile2, *go_home, *check;

/*
*	Funkcne prototypy
*/
bool parse_args(int p_argc, char *p_argv[],int *p_P, int *p_H, int *p_S, int *p_R); // parsovanie argumentov
int file_open(void);  // otvorenie suboru
int file_close(void); // zatvorenie suboru
bool memory_allocation(void); // alokacia zdielanej pamate
void memory_deallocation(void);	// dealokacia zdielanej pamate
void semaphore_open(void); // vytvorenie pomenovanych semaforov
void semaphore_unlink(void); // unlink semaforov
int process_generator(int p_N, int p_GH, int p_GO, int p_B); // vytvaranie procesov
int oxygen_generator(int ID, char *Category,int p_B, int p_N); // pod-vytvaranie kyslikov
int hydrogen_generator(int ID, char *Category,int p_B, int p_N); // pod-vytvaranie vodikov 
void bond(int ID, char *Category, int p_B); // funkcia bond
int wait_for_child(int status); // cakanie na potomka
/*
*	main
*/
int main(int argc, char *argv[]) {
	int N, GH, GO, B = 0;	// argumenty
	if (parse_args(argc, argv, &N, &GH, &GO, &B) == true) {	//ak su nespravne argumenty, tak koniec programu
		if (memory_allocation() == true){	//ak sa nealokovala pamat, tak koniec programu
			semaphore_open();	//vytvorim si potrebne semafory
			file_open();	//otvorim si (vytvorim) vystupny subor
			process_generator(N,GH,GO,B);	//vytvaranie procesov
			file_close();	//zatvorim vystupny subor
			semaphore_unlink();	//unlink vsetkych pouzitych semaforov
			memory_deallocation();	//upratujem po sebe "smeti"
		}
		else {
			return 1;
		}
	}
	else {
		return 1;
	}
}
/*
*	Definicie funkcii
*/
bool parse_args(int p_argc, char *p_argv[], int *p_N, int *p_GH, int *p_GO, int *p_B) {
	char *endptr;
	if (p_argc!=5) {	// pocet argumentov
		fprintf(stderr,"%s",ErrCode[bad_arg_count]);
		return false;
	}
	*p_N = strtol(p_argv[1], &endptr, 10); 	//**endptr - referencia na objekt typu *char, kde  moze funkcia ulozit pointer na miesto kde najde prvy
                    						//neciselny znak v danom stringu - funkcii predavam *endptr na zistenie ci dany parameter je iba cislo
	if ((*endptr != '\0') || (*p_N <= 0)) {
		fprintf(stderr,"%s",ErrCode[bad_arg_N]);
		return false;
	}												

	*p_GH = strtol(p_argv[2], &endptr, 10);
	if ((*endptr != '\0') || (*p_GH < 0) || (*p_GH > 5000)) {
		fprintf(stderr,"%s",ErrCode[bad_arg_GN]);
		return false;			
	}

	*p_GO = strtol(p_argv[3], &endptr, 10);
	if ((*endptr != '\0') || (*p_GO < 0) || (*p_GO > 5000)) {
		fprintf(stderr,"%s",ErrCode[bad_arg_GO]);
		return false;			
	}

	*p_B = strtol(p_argv[4], &endptr, 10);
	if ((*endptr != '\0') || (*p_B < 0) || (*p_B > 5000)) {
		fprintf(stderr,"%s",ErrCode[bad_arg_B]);
		return false;			
	}

	return true; // ak true tak argumenty boli zadane spravne
}

int process_generator(int p_N, int p_GH, int p_GO, int p_B) {
	pid_t child_oxygen_pid, child_hydrogen_pid, pid;	// identifikatory procesov
	int i,j; 
	int child_status = 0;
	pid = fork(); //vytvorenie procesu, ktory vytvori parenta a child
	if (pid == 0) {	//ak prebehlo vytvorenie OK
		srandom(time(NULL)*getpid());
		for (i=0;i<p_N;i++) {	//cyklicke generovanie kyslikov
			child_oxygen_pid = fork();	// vytvorenie kysliku
			if (child_oxygen_pid == 0) {
				oxygen_generator(i+1, "O", p_B, p_N);	
				exit(0);	//ukoncim proces
			}
			else if (child_oxygen_pid == -1) {	//ak nastala chyba
				fprintf(stderr,"%s",ErrCode[process_fail]);	
				exit(1);	//ukonci parent s err code
				return 1;	//vrati chybovy kod
			}
			usleep((random() % (p_GO + 1))*1000);	//uspi proces na dobu podla zadaneho argumentu
		}
		for (i=0; i<p_N; i++){	//cyklicke ukoncovanie procesov
			wait_for_child(child_status);	//synchronizovane ukoncovanie pre kazdy proces kysliku
		}
		exit(0);	// ukoncenie generatora kysliku
	}
	else if (pid == -1) {	//ak nastala chyba pri generovani kysliku															
		fprintf(stderr,"%s",ErrCode[process_fail]);	
		exit(1);	//exit parenta
		return 1;	// chybovy kod
	}
	else {	//parent vetva	
		pid = fork();	//vytvaranie vodikov je obdobne ako pri kyslikoch
		if (pid == 0) {	
			srandom(time(NULL)*getpid());
			for (j=0; j<p_N*2; j++) {
				child_hydrogen_pid = fork();
				if (child_hydrogen_pid == 0) {
					hydrogen_generator(j+1, "H", p_B, p_N*2); // vytvaranie vodikov
					exit(0);	
				}
				else if (child_hydrogen_pid == -1) {	
					fprintf(stderr,"%s",ErrCode[process_fail]);	
					exit(1);	
					return 1;	
				}
					usleep((random() % (p_GH + 1))*1000);	
			}
			for (j=0;j<p_N*2;j++) {
				wait_for_child(child_status);	
			}
			exit(0);
		}
		else if (pid == -1) {
			fprintf(stderr,"%s",ErrCode[process_fail]);	
			exit(1);	
			return 1;		
		}
		wait_for_child(child_status);		
	}
	wait_for_child(child_status);
	return 0;	
}

int wait_for_child(int status) {
	wait(&status);	//ncaka na ukoncenie child
  	if (!(WIFEXITED(status))){												
    fprintf(stderr,"%s",ErrCode[exit_fail]);	
    return 1;	// chybovy kod
  	}
  	return 0;
}

bool memory_allocation(void) { // alokacia zdielanej pamate
	ID_shared_mem = shmget(IPC_PRIVATE, sizeof(struct shared_var), IPC_CREAT | 0664);//odkaz na strukturu
	if (ID_shared_mem == -1){	//ak error pri alokaci
		fprintf(stderr,"%s",ErrCode[memory_fail]);
		return false;
	}
	else{
		shared_data = shmat(ID_shared_mem, NULL, 0);	//ID_shared_mem addresove miesto procesu													
		return true;
	}
}

void memory_deallocation(void) {
	shmdt(shared_data);		//dealokacia struktury
	shmctl(ID_shared_mem,IPC_RMID,NULL);	//dealokacia zdielanej pamate
	return;
}

int file_open(void) {
	if((file=fopen("h2o.out", "w")) == NULL) {	//ak sa nepodari otvorit subor
        fprintf(stderr,"%s",ErrCode[file_open_error]);
        return 2;
    }
    else{
    	setbuf(file, NULL); 
    	return 0;
    }
}

int file_close(void) {
    if((fclose(file)) == EOF) {	//ak sa nepodari uzavriet subor  
    	fprintf(stderr,"%s",ErrCode[file_close_error]);
    	return 2;
    }	
    else{
    	return 0;
    }
}

void semaphore_open(void) {	
	Semaphore_write= sem_open("Semaphore_write", O_CREAT | O_EXCL,0664,1);	//zapisovanie do suboru
	Semaphore_barrier = sem_open("Semaphore_barrier", O_CREAT | O_EXCL,0664,1); //vstupna bariera pre 1 proces				
	Semaphore_shared_mem = sem_open("Semaphore_shared_mem", O_CREAT | O_EXCL,0664,1);	//pristup do pamate
	Semaphore_bonding = sem_open("Semaphore_bonding", O_CREAT | O_EXCL,0664,1);	//bonding process
	my_hydro_queue	= sem_open("my_hydro_queue", O_CREAT | O_EXCL,0664,0);	//"rada" vodikov
	my_oxygen_queue = sem_open("my_oxygen_queue", O_CREAT | O_EXCL,0664,0); //"rada" kyslikov
	my_mutex = sem_open("my_mutex", O_CREAT | O_EXCL,0664,1); //!!uz nikdy nenazvat semafor "mutex"!!
	waiting = sem_open("waiting", O_CREAT | O_EXCL,0664,1); //wait
	turnstile1 = sem_open("turnstile1", O_CREAT | O_EXCL,0664,0); // barrier
	turnstile2 = sem_open("turnstile2", O_CREAT | O_EXCL,0664,1); // barrier
	go_home = sem_open("go_home", O_CREAT | O_EXCL,0664,0); 
	check = sem_open("check", O_CREAT | O_EXCL,0664,1);	// cakanie	
	return;
}

void semaphore_unlink(void) {
	sem_unlink("Semaphore_write");
	sem_unlink("Semaphore_barrier");
	sem_unlink("Semaphore_bonding");
	sem_unlink("Semaphore_shared_mem");
	sem_unlink("my_hydro_queue");
	sem_unlink("my_oxygen_queue");
	sem_unlink("my_mutex");
	sem_unlink("waiting");
	sem_unlink("turnstile1");
	sem_unlink("turnstile2");
	sem_unlink("go_home");
	sem_unlink("check");
	return;
}

int oxygen_generator(int ID, char *Category, int p_B, int p_N) {
	sem_wait(my_mutex);
	sem_wait(Semaphore_shared_mem);
	shared_data->line_count++;
	shared_data->status="started";
	sem_wait(Semaphore_write);
	fprintf(file,"%d\t: %s %d\t: %s\n",shared_data->line_count,Category,ID,shared_data->status);
	sem_post(Semaphore_write);
	shared_data->oxygen_wait++;
	sem_post(Semaphore_shared_mem);
	if (shared_data->hydrogen_wait >= 2){
		sem_wait(Semaphore_shared_mem);
		shared_data->line_count++;
		shared_data->status="ready";
		sem_wait(Semaphore_write);
		fprintf(file,"%d\t: %s %d\t: %s\n",shared_data->line_count,Category,ID,shared_data->status);
		sem_post(Semaphore_write);
		sem_post(my_hydro_queue);
		sem_post(my_hydro_queue);
		shared_data->hydrogen_wait=shared_data->hydrogen_wait-2;
		sem_post(my_oxygen_queue);
		shared_data->oxygen_wait=shared_data->oxygen_wait-1;
		sem_post(Semaphore_shared_mem);
	}
	else {
		sem_wait(Semaphore_shared_mem);
		shared_data->line_count++;
		shared_data->status="waiting";
		sem_wait(Semaphore_write);
		fprintf(file,"%d\t: %s %d\t: %s\n",shared_data->line_count,Category,ID,shared_data->status);
		sem_post(Semaphore_write);
		sem_post(Semaphore_shared_mem);		
		sem_post(my_mutex);
	}
	sem_wait(my_oxygen_queue);
	bond(ID,Category, p_B); //START BONDING
	sem_post(my_mutex);

	sem_wait(check);
	if (shared_data->bonded == p_N*3) {
		for(int i = 0; i <= p_N*3; i++) //ak pocet bondovanych procesov = N*3 tak sa pusti semafor check
		{
			sem_post(check);
			sem_post(go_home);
		}
	}
	else {
		sem_post(check);
		sem_wait(go_home);
	}
		sem_wait(Semaphore_shared_mem);
		shared_data->status="finished";
		shared_data->line_count += 1;
		sem_wait(Semaphore_write);
		fprintf(file,"%d\t: %s %d\t: %s\n",shared_data->line_count,Category,ID,shared_data->status);
		sem_post(Semaphore_write);
		sem_post(Semaphore_shared_mem);
	return 0;
}
int hydrogen_generator(int ID, char *Category, int p_B, int p_N) {
	sem_wait(my_mutex);
	sem_wait(Semaphore_shared_mem);
	shared_data->line_count++;
	shared_data->status="started";
	sem_wait(Semaphore_write);
	fprintf(file,"%d\t: %s %d\t: %s\n",shared_data->line_count,Category,ID,shared_data->status);
	sem_post(Semaphore_write);
	shared_data->hydrogen_wait++;
	sem_post(Semaphore_shared_mem);

	if ((shared_data->hydrogen_wait >= 2) && (shared_data->oxygen_wait >= 1)) {
		sem_wait(Semaphore_shared_mem);
		shared_data->status="ready";
		shared_data->line_count++;
		sem_wait(Semaphore_write);
		fprintf(file,"%d\t: %s %d\t: %s\n",shared_data->line_count,Category,ID,shared_data->status);
		sem_post(Semaphore_write);
		sem_post(my_hydro_queue);
		sem_post(my_hydro_queue);
		shared_data->hydrogen_wait=shared_data->hydrogen_wait-2;
		sem_post(my_oxygen_queue);
		shared_data->oxygen_wait=shared_data->oxygen_wait-1;
		sem_post(Semaphore_shared_mem);
	}	
	else {
		sem_wait(Semaphore_shared_mem);
		shared_data->line_count++;
		shared_data->status="waiting";
		sem_wait(Semaphore_write);
		fprintf(file,"%d\t: %s %d\t: %s\n",shared_data->line_count,Category,ID,shared_data->status);
		sem_post(Semaphore_write);
		sem_post(Semaphore_shared_mem);		
		sem_post(my_mutex);
	}
	sem_wait(my_hydro_queue);
	bond(ID,Category, p_B); // BSTART BONDING

	sem_wait(check);
	if (shared_data->bonded == p_N*3) {
		for(int i = 0; i <= p_N*3; i++){
			sem_post(check);
			sem_post(go_home);
		}
	}
	else {
		sem_post(check);
		sem_wait(go_home);
	}
		sem_wait(Semaphore_shared_mem);
		shared_data->status="finished";
		shared_data->line_count += 1;
		sem_wait(Semaphore_write);
		fprintf(file,"%d\t: %s %d\t: %s\n",shared_data->line_count,Category,ID,shared_data->status);
		sem_post(Semaphore_write);
		sem_post(Semaphore_shared_mem);
	return 0;
}
void bond(int ID, char *Category, int p_B) {
	sem_wait(Semaphore_barrier);
	sem_wait(Semaphore_shared_mem);
	shared_data->status="begin bonding";
	shared_data->line_count++;
	sem_wait(Semaphore_write);
	fprintf(file,"%d\t: %s %d\t: %s\n",shared_data->line_count,Category,ID,shared_data->status);	
	sem_post(Semaphore_write);
	sem_post(Semaphore_barrier);
	sem_post(Semaphore_shared_mem);
	usleep((random() % (p_B + 1))*1000);
	///////////////////////
	sem_wait(waiting); 
	sem_wait(Semaphore_shared_mem);
		shared_data->process_counter++;
	sem_post(Semaphore_shared_mem);
	if (shared_data->process_counter == 3) {
		sem_wait(turnstile2); 
		sem_post(turnstile1); 
	}
	sem_post(waiting);

	sem_wait(turnstile1);
	sem_post(turnstile1);

	sem_wait(Semaphore_shared_mem);
	shared_data->status="bonded";
	shared_data->line_count++;
	shared_data->bonded += 1;
	sem_wait(Semaphore_write);
	fprintf(file,"%d\t: %s %d\t: %s\n",shared_data->line_count,Category,ID,shared_data->status);
	sem_post(Semaphore_write);
	sem_post(Semaphore_shared_mem);

	sem_wait(waiting);
	sem_wait(Semaphore_shared_mem);
		shared_data->process_counter -= 1;
	sem_post(Semaphore_shared_mem);		
	if (shared_data->process_counter == 0) {
		sem_wait(turnstile1);
		sem_post(turnstile2);
	}
	sem_post(waiting);
	sem_wait(turnstile2);
	sem_post(turnstile2);

	return ;
}
/* Koniec suboru h2o.c*/