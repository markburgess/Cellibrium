/*
 * This file derives from code developed at CFEngine AS/Inc. It my be
 * modified and redistributed freely, by permission.
 */

#include <cf3.defs.h>
#include <cf3.extern.h>
#include <probes.h>

/* Structs */

typedef struct
{
   const char *name;
   ProbeInit init;
} Probe;

/* Constants */

static const Probe ENTERPRISE_PROBES[] =
{
    {"Input/output", &MonIoInit},
    {"Memory", &MonMemoryInit},
};

/* Globals */

static ProbeGatherData ENTERPRISE_PROBES_GATHERERS[sizeof(ENTERPRISE_PROBES) / sizeof(ENTERPRISE_PROBES[0])];

/****************************************************************************/

void MonOtherInit(void)
{
 int i;
 
 Log(LOG_LEVEL_VERBOSE, "Starting initialization of static Nova monitoring probes.");
 
 for (i = 0; i < sizeof(ENTERPRISE_PROBES) / sizeof(ENTERPRISE_PROBES[0]); ++i)
    {
    const Probe *probe = &ENTERPRISE_PROBES[i];
    const char *provider;
    const char *error;
    
    if ((ENTERPRISE_PROBES_GATHERERS[i] = (probe->init) (&provider, &error)))
       {
       Log(LOG_LEVEL_VERBOSE, " * %s: %s.", probe->name, provider);
       }
    else
       {
       Log(LOG_LEVEL_VERBOSE, " * %s: Disabled: %s.", probe->name, error);
       }
    }
 
 Log(LOG_LEVEL_VERBOSE, "Initialization of static Nova monitoring probes is finished.");
}

/****************************************************************************/

void MonOtherGatherData(double *cf_this)
{
 int i;
 
 Log(LOG_LEVEL_VERBOSE, "Gathering data from static Nova monitoring probes.");
 
 for (i = 0; i < sizeof(ENTERPRISE_PROBES) / sizeof(ENTERPRISE_PROBES[0]); ++i)
    {
    const char *probename = ENTERPRISE_PROBES[i].name;
    ProbeGatherData gatherer = ENTERPRISE_PROBES_GATHERERS[i];
    
    if (gatherer)
       {
       Log(LOG_LEVEL_VERBOSE, " * %s", probename);
       (*gatherer) (cf_this);
       }
    }
 Log(LOG_LEVEL_VERBOSE, "Gathering data from static Nova monitoring probes is finished.");
}
