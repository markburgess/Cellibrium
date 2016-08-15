/*
 * This file derives from code developed at CFEngine AS/Inc. It my be
 * modified and redistributed freely, by permission.
 */

#include <cf3.defs.h>
#include <cf3.extern.h>
#include <cf.enterprise.h>
#include <shared_kstat.h>

static kstat_ctl_t *kstat;

kstat_ctl_t *GetKstatHandle(void)
{
 if (!kstat)
    {
    kstat = kstat_open();
    if (!kstat)
       {
       Log(LOG_LEVEL_ERR, "kstat_open", "Unable to open Solaris kstat subsystem");
       }
    }
 else
    {
    kstat_chain_update(kstat);
    }
 
 return kstat;
}
