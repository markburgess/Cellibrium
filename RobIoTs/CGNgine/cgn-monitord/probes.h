/*****************************************************************************/
/*                                                                           */
/* File: probes.h                                                            */
/*                                                                           */
/* Created: Sun Sep  6 17:29:18 2015                                         */
/*                                                                           */
/*****************************************************************************/

#include "cf3.defs.h"
#include "cf3.extern.h"


void MonOtherInit(void);
void MonOtherGatherData(double *cf_this);

/*
 * Type of callback collecting actual probe data.
 */
typedef void (*ProbeGatherData) (double *cf_this);

/*
 * Type of probe initialization function.
 *
 * Probe initialization function should either return callback and name of probe
 * provider in "name" argument, or NULL and error description in "error"
 * argument.
 *
 * Caller does not free data returned in "name" or "error".
 */
typedef ProbeGatherData(*ProbeInit) (const char **name, const char **error);

/*
 * Existing probes and their identifiers
 */

#define MON_IO_READS "io_read"
#define MON_IO_WRITES "io_write"
#define MON_IO_READDATA "io_r_data"
#define MON_IO_WRITTENDATA "io_w_data"

ProbeGatherData MonIoInit(const char **name, const char **error);

#define MON_MEM_TOTAL "mem_total"
#define MON_MEM_FREE "mem_free"
#define MON_MEM_CACHED "mem_cached"
#define MON_MEM_SWAP "swap_alloc"
#define MON_MEM_FREE_SWAP "swap_free"

ProbeGatherData MonMemoryInit(const char **name, const char **error);

