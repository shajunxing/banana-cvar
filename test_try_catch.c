#include <setjmp.h>
#include <stdio.h>

#define try                            \
    do {                               \
        jmp_buf __calling_environment; \
        int ex;                        \
        if (!(ex = setjmp(__calling_environment)))
#define end \
    }       \
    while (0)
#define throw(ex) longjmp(__calling_environment, ex)

int main() {
    {
        int i = 10;
        printf("%d\n", i);
        {
            int i = 20;
            printf("%d\n", i);
        }
        printf("%d\n", i);
    }

    try {
        printf("Inside Try statement \n");

        try {
            printf("Inside Try statement \n");
            throw(3);
            printf("This does not appear as exception has already been called \n");
        }
        else {
            printf("Exception %d called \n", ex);
        }
        end;

        throw(2);
        printf("This does not appear as exception has already been called \n");
    }
    else {
        printf("Exception %d called \n", ex);
    }
    end;

    return 0;
}