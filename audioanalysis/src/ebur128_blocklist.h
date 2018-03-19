#ifndef EBUR128_BLOCKLIST_H
#define EBUR128_BLOCKLIST_H

/* This can be replaced by any BSD-like queue implementation. */
#include <sys/queue.h>
#include <stdlib.h>

#define __DQ_MEMORY_SIZE 1024*16

SLIST_HEAD(ebur128_double_queue_global, ebur128_dq_entry_global);
struct ebur128_dq_entry_global {
  double z[__DQ_MEMORY_SIZE]; /* 27 min chunk */
  double *pt;
  double *end_pt;
  SLIST_ENTRY(ebur128_dq_entry_global) entries;
};

void
blocklist_init   (struct ebur128_double_queue_global*);

void
blocklist_append (struct ebur128_double_queue_global*,
		  double);

/*double
 *blocklist_sum    (struct ebur128_double_queue_global*);
 */

void
blocklist_sum_size  (struct ebur128_double_queue_global*,
		     double*,
		     size_t*);

void
blocklist_sum_size_if_gt_or_eq  (struct ebur128_double_queue_global*,
				 double*,
				 double*,
				 size_t*);

void
blocklist_clear  (struct ebur128_double_queue_global*);

void
blocklist_delete (struct ebur128_double_queue_global*);

#endif /* EBUR128_BLOCKLIST_H */
