#include <stdio.h>
#include <unistd.h>

#define PAUSE do {usleep(250000);} while (0)

int main() {
    const char *pathin = "in.txt";
    const char *pathout = "out.txt";
    FILE *in, *out;
    
    PAUSE;
    in = fopen(pathin, "r");
    PAUSE;
    out = fopen(pathout, "w");
    PAUSE;
    fclose(in);
    PAUSE;
    fclose(out);
    PAUSE;
    in = fopen(pathin, "r");
    PAUSE;
    fclose(in);
    PAUSE;
    out = fopen(pathout, "w");
    PAUSE;
    fclose(out);
    
    return 0;
}

