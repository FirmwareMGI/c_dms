#include <stdio.h>
#include <unistd.h>  // for sleep()

int main(int argc, char *argv[]) {
    printf("Program started with %d argument(s):\n", argc - 1);
    for (int i = 1; i < argc; ++i) {
        printf("  Argument %d: %s\n", i, argv[i]);
    }

    // Infinite loop
    while (1) {
        printf("Still running with PID %d...\n", getpid());
        sleep(5);  // Sleep 5 seconds to avoid CPU overuse
    }

    return 0;
}
