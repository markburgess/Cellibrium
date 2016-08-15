#ifndef CFENGINE_SHARED_KSTAT_H
#define CFENGINE_SHARED_KSTAT_H

#include <kstat.h>

/* Shared access to Solaris kstat service */

kstat_ctl_t *GetKstatHandle(void);

#endif
