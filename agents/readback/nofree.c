/* LD_PRELOAD shim for the read-back adapter: free() does nothing, so that node addresses are never
   reused during a run and the identities read from the book table stay unambiguous.
   Memory is returned to the system only when the process ends. */
#include <stddef.h>
void free(void *p) { (void)p; }
