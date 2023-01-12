#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int 
main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "argument number is invalid\n");
        exit(1);
    }
    char *test_p = "asdb1235bca123";
    char *test_p1 = "123 sd 33";
    long test_l = strtol(test_p, &test_p, 10); 
    long test_l1 = strtol(test_p1, &test_p1, 10); 
    printf("[%ld][%ld]\n", test_l, test_l1);
    char *p = argv[1];
    printf("%c\n", *p);
    while (*p++) {
        if (isdigit(*p)) {
            printf("%c", *p);
            long val = strtoul(p, &p, 10); 
            printf("[%ld]", val);
            const char *prev_ptr = p;
            int len = p - prev_ptr;
            printf("(%x)", len);
              
    }}

    return 0;
}
