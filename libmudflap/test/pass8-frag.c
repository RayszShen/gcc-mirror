char *foo;
char *bar;
foo = (char *)malloc (10);
bar = (char *)malloc (10);

free(bar);
bar = (char *)malloc (10);

memcpy(foo, bar, 10);
