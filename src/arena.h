#ifndef KIRA_ARENA_H_
#define KIRA_ARENA_H_

#include <stddef.h>

struct arena_segment;

struct arena {
  char *buf;
  /* offset starts at the size of buf, then decreases. */
  size_t offset;
  struct arena_segment *old_segments;
};

void arena_init(struct arena *a);
void arena_init_move(struct arena *a, struct arena *movee);
void arena_destroy(struct arena *a);

void *arena_unaligned(struct arena *a, size_t count);


#endif /* KIRA_ARENA_H_ */
