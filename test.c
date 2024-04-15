#include "var.h"
#include <locale.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define tt(a) xstr(typeof(a));

void main() {
    __auto_type i = 10;
    adeclare(a);
    printf("%d\n", vtype(a));
    // for (;;) {
    apush(a, onew());
    apush(a, anew(3, snew("zzz"), nnew(99), bnew(true)));
    apush(a, anew(2, nnew(33), bnew(true)));
    apush(a, snew("foobar"));
    apush(a, snew("foo"));
    apush(a, nnew(10));
    apush(a, nnew(9));
    apush(a, nnew(1));
    apush(a, bnew(true));
    apush(a, bnew(false));
    apush(a, znew());
    puts(tojson(a));
    asort(a, NULL);
    puts(tojson(a));
    aclear(a);
    gc();
    // }
}
