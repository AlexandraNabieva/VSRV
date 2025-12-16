#include "msgqueue.h"
#include <unistd.h>

queue_t message_queue;

void *producer_func(void *arg) {
    int id = *(int*)arg;
    char msg[100];
    int i;
    
    printf("Производитель #%d запущен.\n", id);

    for (i = 0; i < 20; i++) {
        snprintf(msg, sizeof(msg), "[%d-%d] Сообщение от Производителя #%d. Тестовая строка для обрезки: XXXXXXXXXXXXXXXXXXXXXXXXXXX", id, i, id);
        
        int len = mymsgput(&message_queue, msg);
        
        if (len > 0) {
            printf("  [P%d] -> Отправил: '%s...' (%d символов)\n", id, msg, len);
        } else if (message_queue.dropped) {
            printf("  [P%d] Отменено из-за drop (попытка #%d)\n", id, i);
            break;
        }
        
        usleep(rand() % 200000);
    }
    
    printf("Производитель #%d завершил работу.\n", id);
    return NULL;
}

void *consumer_func(void *arg) {
    int id = *(int*)arg;
    char buf[41]; 
    int i;
    
    printf("Потребитель #%d запущен.\n", id);

    for (i = 0; i < 30; i++) {
        int len = mymsgget(&message_queue, buf, sizeof(buf));
        
        if (len > 0) {
            printf("  [C%d] <- Получил: '%s' (%d символов) | Длина очереди: %d\n", 
                   id, buf, len, message_queue.count);
        } else if (message_queue.dropped) {
            printf("  [C%d] Отменено из-за drop (попытка #%d)\n", id, i);
            break;
        }

        usleep(rand() % 300000);
    }
    
    printf("Потребитель #%d завершил работу.\n", id);
    return NULL;
}

int main() {
    srand(time(NULL)); 

    mymsginit(&message_queue);

    pthread_t producers[2];
    pthread_t consumers[2];
    int producer_ids[2] = {1, 2};
    int consumer_ids[2] = {1, 2};

    printf("--- Запуск 2 Производителей и 2 Потребителей ---\n");
    
    int i;
    for (i = 0; i < 2; i++) {
        pthread_create(&producers[i], NULL, producer_func, &producer_ids[i]);
        pthread_create(&consumers[i], NULL, consumer_func, &consumer_ids[i]);
    }
    
    sleep(4);

    for (i = 0; i < 2; i++) {
        pthread_join(producers[i], NULL);
        pthread_join(consumers[i], NULL);
    }

    printf("\n--- Все нити завершили работу ---\n");
    
    mymsgdestroy(&message_queue);

    return 0;
}