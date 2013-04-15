#!/bin/sh
gcc shamirssecret.c -O2 -std=c99 -DTEST -o shamirssecret && ./shamirssecret &&
gcc shamirssecret.c -O2 -std=c99 -o shamirssecret
