#include "ebur128_blocklist.h"
#include <stdio.h>

void
blocklist_init    (struct ebur128_double_queue_global* st)
{
  struct ebur128_dq_entry_global* head;
  
  SLIST_INIT(st);
  
  head = (struct ebur128_dq_entry_global*)
    malloc(sizeof(struct ebur128_dq_entry_global));
  head->pt = head->z;
  head->end_pt = head->z + __DQ_MEMORY_SIZE;

  SLIST_INSERT_HEAD(st, head, entries);
}

void
blocklist_append (struct ebur128_double_queue_global* st,
		  double x)
{
  struct ebur128_dq_entry_global* head;

  head = SLIST_FIRST(st);
  /* end of buffer */
  if (head->pt == head->end_pt) {

    head = (struct ebur128_dq_entry_global*)
      malloc(sizeof(struct ebur128_dq_entry_global));

    head->pt = head->z;
    head->end_pt = head->z + __DQ_MEMORY_SIZE;

    SLIST_INSERT_HEAD(st, head, entries);
  }

  /* add val */
  *head->pt = x;
  head->pt++;
}

/*double
 *blocklist_sum    (struct ebur128_double_queue_global* st)
 *{
 *
 *}
 */

void
blocklist_sum_size  (struct ebur128_double_queue_global* st,
		     double* sum,
		     size_t* size)
{
  struct ebur128_dq_entry_global* entry;
  double *val;
  
  SLIST_FOREACH(entry, st, entries) {
    for (val = entry->z; val < entry->pt; val++) {
      ++*size;
      *sum += *val;
    }
  }
}

void
blocklist_sum_size_if_gt_or_eq  (struct ebur128_double_queue_global* st,
				 double* sum,
				 double* boundary,
				 size_t* size)
{
  struct ebur128_dq_entry_global* entry;
  double *val;

  SLIST_FOREACH(entry, st, entries) {
    for (val = entry->z; val < entry->pt; val++) {
      if (*val >= *boundary) {
	++*size;
	*sum += *val;
      }
    }
  }
}

void
blocklist_clear  (struct ebur128_double_queue_global* st)
{
  struct ebur128_dq_entry_global* entry;

  /* till 1 buffer remained */
  
  while ( (SLIST_FIRST(st))->entries.sle_next !=  NULL) {
    entry = SLIST_FIRST(st);
    SLIST_REMOVE_HEAD(st, entries);
    free(entry);
  }

  entry = SLIST_FIRST(st);
  entry->pt = entry->z;
}

void
blocklist_delete (struct ebur128_double_queue_global* st)
{
  struct ebur128_dq_entry_global* entry;
  
  while (!SLIST_EMPTY(st)) {
    entry = SLIST_FIRST(st);
    SLIST_REMOVE_HEAD(st, entries);
    free(entry);
  }
}
