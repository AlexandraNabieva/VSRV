#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int remove_lowercase(char* str){
    int count = 0;
    int j = 0;
    int len = strlen(str);

    for (int i = 0; i<len; i++){
        if (islower(str[i])){
            count++;
        }
        else{
            str[j] = str[i];
            j++;
        }
    }
    str[j] = '\0';

    return count;
}

int main(){
    char* str = (char*)malloc(100*sizeof(char));
    printf("Введите строку: ");
    fgets(str , 100 , stdin);

    int count = remove_lowercase(str);
    printf("Строка после редактирования: %s" , str);
    printf("Количество удаленных символов: %d\n" , count);

    free(str);
    return 0;
}