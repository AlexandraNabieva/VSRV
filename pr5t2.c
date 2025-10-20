#include <stdio.h>
#include <stdlib.h>

int main(){
    int N,M;
    printf("Введите размер двумерного массива: ");
    scanf("%d %d", &N, &M);

    int **array = (int**)malloc(N * sizeof(int*));
    for (int i = 0; i < N; i++){
        array[i] = (int*)malloc(M * sizeof(int));
    }

    if(array == NULL){
        return 0;
    }

    for (int i = 0; i<N; i++){
        for(int j = 0; j<M; j++){
            array[i][j] = (i*10) + (j+1);
            printf("array[%d][%d] = %d, адрес = %p\n" , i , j , array[i][j] , (void*)&array[i][j]);
        }
    }

    for (int i = 0; i<N ; i++){
        free(array[i]);
    }

    free(array);
    return 0;
}