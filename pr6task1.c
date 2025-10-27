#include <stdio.h>
#include <stdlib.h>

void never_called_function(void) {
    printf("[*] Unexpected: This function was called!\n");
    exit(1);
}

void mandatory_function(void) {
    printf("[*] Mandatory function executed\n");
}

void print_arguments(int argc, char **argv) {
    printf("[*] Number of arguments: %d\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("[*] Argument %d: %s\n", i, argv[i]);
    }
}

int main(int argc, char **argv) {
    printf("[*] Program started successfully!\n");
    mandatory_function();
    print_arguments(argc, argv);
    return 0;
}