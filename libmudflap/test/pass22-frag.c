struct foo {
  unsigned base:8;
  unsigned flag1:1;
  unsigned flag2:3;
  unsigned flag3:4;
  char nothing[0];
};

#define offsetof(TYPE, MEMBER)	((size_t) &((TYPE *) 0)->MEMBER)

struct foo* f = (void *) malloc (offsetof (struct foo, nothing));
f->base = 1;
f->flag1 = 1;
free (f);


