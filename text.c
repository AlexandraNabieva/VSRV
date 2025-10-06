#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(){
    char str[50];
    printf("Введите строку (для завершения работы введите - exit)\n");
    scanf("%s" , str);
    while(strcmp("exit" , str)){
        printf("%s\n" , str);
        scanf("%s" , str);
    }
    return 0;
}