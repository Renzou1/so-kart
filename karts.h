#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>

#define KARTS_NUMBER 10
#define HELMETS_NUMBER 10
#define MAX_NAMES 10
#define MAX_SURNAMES 10
#define MAX_NAME_LENGTH 10
#define END_OF_DAY 60 * 8

void* act(void* person_arg);

// MAX_NAME_LENGTH + 1 is to include the null terminated character
typedef struct Person{
    char*        name;
    unsigned int age;
}Person;

typedef struct Kartodromo{
    sem_t karts;
    sem_t helmets;

}Kartodromo;

typedef struct Report_data{
    unsigned int karts_used;
    unsigned int helmets_used;
    unsigned int total_clients;
    unsigned int average_wait;
    unsigned int total_wait;
    unsigned int unanswered_clients;
    unsigned int total_wait_unanswared_clients;
}Report_data;

pthread_mutex_t mutex;

Kartodromo kartodromo;

unsigned int minutes = 0;

Report_data report_data = {.karts_used = 0, .helmets_used = 0, .total_clients = 0,
                            .average_wait = 0, .total_wait = 0,.unanswered_clients = 0,
                            .total_wait_unanswared_clients = 0};

const bool CHILDRENS_DAY = true;
char name_options[MAX_NAMES][MAX_NAME_LENGTH + 1] = {
    "Joao", 
    "Gabriel", 
    "Maria", 
    "Elisa",
    "Ana", 
    "Carlos", 
    "Bruno", 
    "Julia", 
    "Paulo", 
    "Luisa"
};

char surname_options[MAX_SURNAMES][MAX_NAME_LENGTH + 1] = {
    " Silva", 
    " Santos", 
    " Oliveira", 
    " Pereira", 
    " Costa", 
    " Ferreira", 
    " Almeida", 
    " Ribeiro", 
    " Carvalho", 
    " Souza"
};

void sync_init(pthread_mutex_t* mutex, sem_t* karts, sem_t* helmets){
    pthread_mutex_init(mutex, NULL);
    sem_init(karts, 0, 10);
    sem_init(helmets, 0, 10);
}

void sync_destroy(pthread_mutex_t* mutex, sem_t* karts, sem_t* helmets){
    pthread_mutex_destroy(mutex);
    sem_destroy(karts);
    sem_destroy(helmets);
}

bool is_child(const Person* person){
    return person->age < 18;
}

bool is_priority_child(const Person* person){
    return person->age <= 14;
}

void get_helmet(const Person* person){
//    printf("    %s esperando capacete\n", person->name);
    sem_wait(&kartodromo.helmets);
//    printf("    %s pegou capacete.\n", person->name);

    pthread_mutex_lock(&mutex);
    report_data.helmets_used++;
    pthread_mutex_unlock(&mutex);
}

void get_kart(const Person* person){
//    printf("    %s esperando kart\n", person->name);
    sem_wait(&kartodromo.karts);
//    printf("    %s pegou kart.\n", person->name);

    pthread_mutex_lock(&mutex);
    report_data.karts_used++;
    pthread_mutex_unlock(&mutex);
}

void release_helmet(){
    sem_post(&kartodromo.helmets);
}

void release_kart(){
    sem_post(&kartodromo.karts);
}

void enter_track(const Person* person){
    printf("    %s(%d) entrou na pista.\n", person->name, person->age);
}

void leave_track(const Person* person){
    printf("    %s(%d) saiu da pista.\n", person->name, person->age);
}

Person* new_person(){
    Person* person = (Person*)malloc(sizeof(Person));

    int selected = rand() % MAX_NAMES;
    char* surname = strdup(surname_options[selected]);

    selected = rand() % MAX_NAMES;
    person->name = strdup(name_options[selected]);

    strcat(person->name, surname);
    free(surname);
    int minimum_age = 5;
    int max_age;

    bool is_child = rand() % 2 == 0; // 50% das pessoas sao criancas
    if (is_child) max_age = 17;
    else max_age = 60;
    
    person->age = minimum_age + rand() % (max_age - minimum_age);

    return person;
}

pthread_t person_arrive(){
    Person* person = new_person();
    pthread_t new_thread;
    report_data.total_clients++;
    pthread_create(&new_thread, NULL, act, (void*)person);

    return new_thread;
}

void person_leave(Person* person){
    printf("    %s(%d) saiu do kartodromo.\n", person->name, person->age);
    free(person->name);
    free(person);
}

unsigned int priority_children_in_queue = 0;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

void* act(void* person_arg){
    Person* person = (Person*) person_arg;
    printf("    %s(%d) chegou no kartodromo.\n", person->name, person->age);

    unsigned int start_time = minutes;
    if(is_child(person)){
        // Na fila para pegar capacetes, crianças até 14 anos possuem prioridade em relação as outras crianças.
        if(is_priority_child(person)){
            pthread_mutex_lock(&mutex);
            priority_children_in_queue++;
            pthread_mutex_unlock(&mutex);

            get_helmet(person);

            pthread_mutex_lock(&mutex);
            priority_children_in_queue--;
            if(priority_children_in_queue == 0){
                pthread_cond_broadcast(&condition);
            }
            pthread_mutex_unlock(&mutex);

        }  else{
            pthread_mutex_lock(&mutex);            
            while(priority_children_in_queue > 0){
                pthread_cond_wait(&condition, &mutex);
            }
            pthread_mutex_unlock(&mutex);

            get_helmet(person);
        }
        get_kart(person);

    }  else{
        get_kart(person);
        get_helmet(person);

    }
    unsigned int end_time = minutes;
    pthread_mutex_lock(&mutex);            
    report_data.total_wait += end_time - start_time;

    if(minutes == END_OF_DAY){
        report_data.unanswered_clients++;
        report_data.total_wait_unanswared_clients += end_time - start_time;
        release_helmet();
        release_kart();  

        person_leave(person);
        pthread_mutex_unlock(&mutex);

    }  else {
        pthread_mutex_unlock(&mutex);

        enter_track(person);
        const int initial_time = minutes;
        const int minimum_time = 15;
        const int max_time = 60;
        int desired_time = minimum_time + rand() % (max_time - minimum_time);

        while(minutes < initial_time + desired_time && minutes != END_OF_DAY){
            sleep(1);
        }

        leave_track(person);

        release_helmet();
        release_kart();

        person_leave(person);        
    }
    

}



void group_arrive(int* thread_counter, pthread_t thread_list[]){
    int max_people = 4;
    int group_size = 1 + rand() % (max_people - 1);

    for (int i = 0; i < group_size; i++){
        thread_list[*thread_counter] = person_arrive();
        (*thread_counter)++;
    }
}

void write_report(){
    FILE *fp = fopen("report.txt", "w");

    fprintf(fp, "Numero de clientes: %d\n",               report_data.total_clients);
    fprintf(fp, "Total de capacetes usados: %d\n",        report_data.helmets_used);
    fprintf(fp, "Total de karts usados: %d\n",            report_data.karts_used);
    fprintf(fp, "Tempo de espera medio dos clientes que foram atendidos: %.2f minutos.\n", report_data.total_wait / (float) report_data.total_clients);
    fprintf(fp, "Tempo de espera total dos clientes que foram atendidos: %d minutos.\n",   report_data.total_wait);
    fprintf(fp, "Clientes que nao foram atendidos: %d.\n",   report_data.unanswered_clients);
    fprintf(fp, "Tempo de espera total dos clientes que nao foram atendidos: %d minutos.\n",   report_data.total_wait_unanswared_clients);
    fprintf(fp, "Tempo de espera medio dos clientes que nao foram atendidos: %.2f minutos.\n",   report_data.total_wait_unanswared_clients / (float) report_data.unanswered_clients);

    fclose(fp);
}