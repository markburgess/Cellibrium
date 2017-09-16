/*
   Copyright (C) CFEngine AS

   This file is part of CFEngine 3 - written and maintained by CFEngine AS.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#include <cf3.defs.h>
#include <mon.h>
#include <item_lib.h>
#include <files_interfaces.h>
#include <pipes.h>
#include <systype.h>
#include <string_lib.h>
#include <cf-windows-functions.h>
#include <processes_select.h>

/* Prototypes */

#ifndef __MINGW32__
static bool GatherProcessUsers(Item **userList, int *userListSz, int *numRootProcs, int *numOtherProcs);
#endif

/* Implementation */

void MonProcessesGatherData(double *cf_this)
{
 Item *userList = NULL;
 char vbuff[CF_BUFSIZE];
 int numProcUsers = 0;
 int numRootProcs = 0;
 int numOtherProcs = 0;
 
 if (!GatherProcessUsers(&userList, &numProcUsers, &numRootProcs, &numOtherProcs))
    {
    return;
    }
 
 cf_this[ob_users] += numProcUsers;
 cf_this[ob_rootprocs] += numRootProcs;
 cf_this[ob_otherprocs] += numOtherProcs;
 
 snprintf(vbuff, CF_MAXVARSIZE, "%s/state/cf_users", CFWORKDIR);
 MapName(vbuff);
 RawSaveItemList(userList, vbuff, NewLineMode_Unix);
 
 DeleteItemList(userList);
 
 Log(LOG_LEVEL_VERBOSE, "[0] (Users,root,other) = (%d,%d,%d)", (int) cf_this[ob_users], (int) cf_this[ob_rootprocs],
     (int) cf_this[ob_otherprocs]);
}

/**************************************************************************************/

#ifndef __MINGW32__

static bool GatherProcessUsers(Item **userList, int *userListSz, int *numRootProcs, int *numOtherProcs)
{
 FILE *pp;
 char pscomm[CF_BUFSIZE];
 char user[CF_MAXVARSIZE];
 
 snprintf(pscomm, CF_BUFSIZE, "%s %s", VPSCOMM[VPSHARDCLASS], VPSOPTS[VPSHARDCLASS]);
 
 if ((pp = cf_popen(pscomm, "r", true)) == NULL)
    {
    /* FIXME: no logging */
    return false;
    }
 
 size_t vbuff_size = CF_BUFSIZE;
 char *vbuff = xmalloc(vbuff_size);
 
 /* Ignore first line -- header */
 ssize_t res = CfReadLine(&vbuff, &vbuff_size, pp);
 if (res <= 0)
    {
    /* FIXME: no logging */
    cf_pclose(pp);
    free(vbuff);
    return false;
    }
 
 for (;;)
    {
    ssize_t res = CfReadLine(&vbuff, &vbuff_size, pp);
    if (res == -1)
       {
       if (!feof(pp))
          {
          /* FIXME: no logging */
          cf_pclose(pp);
          free(vbuff);
          return false;
          }
       else
          {
          break;
          }
       }
    
    sscanf(vbuff, "%s", user);
    
    if (strcmp(user, "USER") == 0)
       {
       continue;
       }
    
    if (!IsItemIn(*userList, user))
       {
       PrependItem(userList, user, NULL);
       (*userListSz)++;
       }
    
    if (strcmp(user, "root") == 0)
       {
       (*numRootProcs)++;
       }
    else
       {
       (*numOtherProcs)++;
       }
    }
 
 cf_pclose(pp);
 free(vbuff);
 return true;
}

/***********************************************************************************************/

int MonClassifyProcessState()
{
 // This is GNU/Linux only
 FILE *prp;
 Item *processtable = NULL, *ip;
 char pscomm[CF_MAXVARSIZE]; 
 snprintf(pscomm, CF_MAXLINKSIZE, "/bin/ps -eo user,ppid,pgid,pcpu,pmem,vsz,ni,rss:9,nlwp,stime,args");

 if ((prp = cf_popen(pscomm, "r", false)) == NULL)
    {
    Log(LOG_LEVEL_ERR, "Couldn't open the process list with command '%s'. (popen: %s)", pscomm, GetErrorStr());
    return false;
    }

 char *column[CF_PROCCOLS] = {0};
 char *names[CF_PROCCOLS] = {0};
 int start[CF_PROCCOLS] = {0};
 int i,end[CF_PROCCOLS] = {0};
 char line[CF_BUFSIZE];

 while (!feof(prp))
    {
    line[0] = '\0';
    fgets(line,CF_BUFSIZE,prp);
    if (strlen(line)>0)
       {
       Chop(line, CF_BUFSIZE);
       AppendItem(&processtable,line,NULL);
       }
    }


 char *titles = processtable->name;
 time_t pstime = time(NULL);
 
 GetProcessColumnNames(titles, &names[0], start, end);

 for (ip = processtable; ip != NULL; ip=ip->next)
    {
    if (!SplitProcLine(ip->name, pstime, names, start, end, column))
       {
       return false;
       }
    
    for (i = 0; names[i] != NULL; i++)
       {
       Log(LOG_LEVEL_DEBUG, "In SelectProcess, COL[%s] = '%s'", names[i], column[i]);
       printf("In SelectProcess, COL[%s] = '%s'\n", names[i], column[i]);
       }

    for (i = 0; column[i] != NULL; i++)
       {
       free(column[i]);
       }
    }
 
 return 1;
}


#endif
