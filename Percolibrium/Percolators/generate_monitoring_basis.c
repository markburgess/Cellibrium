/*****************************************************************************/
/*                                                                           */
/* File: generate_monitoring_basis.c                                         */
/*                                                                           */
/* Created: Tue Aug  8 15:04:56 2017                                         */
/*                                                                           */
/*****************************************************************************/

 // Generate CGNgine style time coding

 // gcc -o gen_monbas -g -std=c99 generate_monitoring_basics.c

 // Usage example: ./gen_monbas > ExampleTupleData/monbas

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>

#define true 1
#define false 0
#define MAX_ASSOC_ARRAY 128

// Import standard link definitions

#include "../../RobIoTs/CGNgine/libpromises/graph.h"
#include "../../RobIoTs/CGNgine/libpromises/graph_defs.c"

/********************************************************************************/

int main()
{
// RoleCluster(stdout,"system monitoring sample","sample", "system monitoring", ContextCluster(stdout,"computer system monitoring"));


 // GetDeltaInvariant();

 // Smart sensors need to produce these clues
 
  char *who = "cgn-monitord";
  char *what = "new open port";
  time_t when = time(NULL);
  char *where = "hostname.example.com";
  char *how = "port 80 listening";
  char *why = "lighttpd process started"; // or "unknown"
  char *icontext = "system monitoring";
  
  Clue(stdout,who,what,when,where,how,why,icontext);

  who = "cgn-monitord";
  what = "notable anomaly";
  when = time(NULL);
  where = "hostname.example.com";
  how = "www_in_high_anomaly cpu_high_anomaly";
  why = "unknown"; // or "unknown"
  icontext = "system monitoring";
  
  Clue(stdout,who,what,when,where,how,why,icontext);

  
 return 0;
}

