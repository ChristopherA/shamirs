#!/bin/sh
$CC shamirssecret.c $CFLAGS -Wall -Werror -O2 -std=c99 -DTEST -o shamirssecret && ./shamirssecret &&
$CC shamirssecret.c $CFLAGS -Wall -Werror -O2 -std=c99 -o shamirssecret
