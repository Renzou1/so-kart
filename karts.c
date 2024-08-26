#include "karts.h"
#define GROUP_CHANCE 4

int main(){
    sync_init(&mutex, &kartodromo.karts, &kartodromo.helmets);
    srand(time(NULL));
    pthread_t thread_list[100];

    int thread_counter = 0;

    bool running = true;
    unsigned int hour_of_day = 12;
    printf("hora: %d\n", hour_of_day);

    while(running){
        minutes++;
        if (rand() % 100 <= GROUP_CHANCE){
            group_arrive(&thread_counter, thread_list);
        }           
        if (minutes % 60 == 0){
            sleep(1);
            hour_of_day++;
            printf("hora: %d\n", hour_of_day);
         
        }
        if (hour_of_day == 20){
            for (int i = 0; i < thread_counter; i++){
                pthread_join(thread_list[i], NULL);

            }
            printf("fim do dia.\n");
            running = false;
        }
    }
    write_report();
    sync_destroy(&mutex, &kartodromo.karts, &kartodromo.helmets);
    pthread_cond_destroy(&condition);
}