#!/bin/sh
$CC $CFLAGS -Wall -Werror -O2 -std=c99 shamirssecret.c -DTEST -o shamirssecret && ./shamirssecret &&
$CC $CFLAGS -Wall -Werror -O2 -c shamirssecret.c -o shamirssecret.o &&
$CC $CFLAGS -Wall -Werror -O2 -std=c99 main.c shamirssecret.o -o shamirssecret
