#include <stdio.h>

int *pa;
char **pb;
double *pc;

void foo() {
    int a = 1;
    char *b = "hello";
    double c = 3.14;
    printf("&foo,&a,&b,&c = %X,%X,%X,%X\n", &foo, &a, &b, &c);
    pa = &a;
    pb = &b;
    pc = &c;
}

void bar() {
    int a = 2;
    char *b = "world";
    double c = 2.71828;
    printf("&bar,&a,&b,&c = %X,%X,%X,%X\n", &bar, &a, &b, &c);
}

int main() {
    foo();
    printf("a,b,c = %d,%s,%f\n", *pa, *pb, *pc);
    bar();
    printf("a,b,c = %d,%s,%f\n", *pa, *pb, *pc);
}