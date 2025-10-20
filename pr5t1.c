#include <stdio.h>
#include <stdlib.h>

int main(){
    int N;
    printf("Введите размер массива: ");
    scanf("%d" , &N);

    int *array = (int*)malloc(N * sizeof(int));
    if (array == NULL){
        return 0;
    }

    for(int i = 0; i<N; i++){
        array[i] = i + 1;
        printf("array[%d] = %d, адрес = %p\n" , i , array[i] , (void*)&array[i]);
    }

    free(array);
    return 0;
}