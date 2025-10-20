#include <stdio.h>
#include <stdbool.h>
#include <limits.h>

int main() {
    int n;

    printf("Введите размер массива: ");
    scanf("%d", &n);
    
    int arr[n];
    printf("Введите %d чисел: ", n);
    for (int i = 0; i < n; i++) {
        scanf("%d", &arr[i]);
    }
    
    int composite_count = 0;
    int max_composite = INT_MIN;
    
    for (int i = 0; i < n; i++) {
        int num = arr[i];
        
        if (num < 4) {
            continue;
        }
        
        bool is_composite = false;
        for (int j = 2; j * j <= num; j++) {
            if (num % j == 0) {
                is_composite = true;
                break;
            }
        }
        
        if (is_composite) {
            composite_count++;
            if (num > max_composite) {
                max_composite = num;
            }
        }
    }
    
    if (composite_count > 0) {
        printf("Количество составных чисел: %d\n", composite_count);
        printf("Наибольшее составное число: %d\n", max_composite);
    } else {
        printf("В массиве нет составных чисел\n");
    }
    
    return 0;
}