/*

   Copyright (C) Cfengine AS

   This file is part of Cfengine 3 - written and maintained by Cfengine AS.

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
  versions of Cfengine, the applicable Commerical Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#include <generic_agent.h>
#include <eval_context.h>
#include <dir.h>
#include <writer.h>
#include <dbm_api.h>
#include <lastseen.h>
#include <granules.h>
#include <files_names.h>
#include <files_hashes.h>
#include <policy.h>
#include <regex.h>          /* CompileRegex,StringMatchWithPrecompiledRegex */
#include <syntax.h>
#include <item_lib.h>
#include <conversion.h>
#include <unix.h>
#include <communication.h>
#include <logging.h>
#include <assert.h>
#include <ornaments.h>
#include <vars.h>
#include <signals.h>
#include <scope.h>
#include <known_dirs.h>
#include <man.h>
#include <bootstrap.h>
#include <timeout.h>
#include <time_classes.h>
#include <loading.h>
#include <string_lib.h>

static void ThisAgentInit(void);
static GenericAgentConfig *CheckOpts(int argc, char **argv);
static void KeepReportsPromises(void);

#ifdef HAVE_QSORT
static int CompareClasses(const void *a, const void *b);
#endif
static void ReadAverages(bool verbose);
static void SummarizeAverages(void);
static void WriteGraphFiles(void);
static void WriteHistograms(void);
static void OpenFiles(void);
static void CloseFiles(void);
static void WriteNetPromises(void);
extern const BodySyntax CFRE_CONTROLBODY[];
char *cf_ctime(const time_t *timep);
char *cf_strtimestamp_local(const time_t time, char *buf);
static char *cf_format_strtimestamp(struct tm *tm, char *buf);
static void OutputSingleMeasure(char *);
static bool OutputWeekly(char *name);
static void OutputSingle(char *name);
static void OutputWeeklyJSON(int slot, char *name, char *desc);
static void ListAll(void);
static void WriteAgentPromises(void);
static void OutputPromises(char *name);
static void HandleURI(char *uri);
static void HandleAgentHandle(char *handle, char *element);
static void HandleMeasurementHandle(char *handle, char *element);
static void ListAgentHandles(void);
static void ListMeasurementHandles(void);

/*******************************************************************/
/* GLOBAL VARIABLES                                                */
/*******************************************************************/

extern BodySyntax CFRP_CONTROLBODY[];

int CSV = false;
int HIRES = false;
int NOSCALING = true;
int NOWOPT = false;
int EMBEDDED = false;
int TIMESTAMPS = false;
int LISTALL = false;

static double HISTOGRAM[CF_OBSERVABLES][7][CF_GRAINS] = { { { 0.0 } } };
static double SMOOTHHISTOGRAM[CF_OBSERVABLES][7][CF_GRAINS] = { { { 0.0 } } };

double AGE;

static Averages MAX, MIN;

char OUTPUTDIR[CF_BUFSIZE];
char BYNAME[CF_BUFSIZE] = {0};
char BYPROMISE[CF_BUFSIZE] = {0};
char URI[CF_BUFSIZE] = {0};
char OVERRIDE[CF_BUFSIZE] = {0};

FILE *FPE[CF_OBSERVABLES], *FPQ[CF_OBSERVABLES];
FILE *FPM[CF_OBSERVABLES];
int HAVE_DATA[CF_OBSERVABLES];

static const double ticksperhr = (double) SECONDS_PER_HOUR;

/*******************************************************************/
/* Command line options                                            */
/*******************************************************************/

static const char *const CF_REPORT_SHORT_DESCRIPTION =
    "output machine learned data collected by CGNgine";

static const char *const CF_REPORT_MANPAGE_LONG_DESCRIPTION =
    "cgn-report is a simple data reporting tool for CGNngine. It extracts data from embedded databases "
    "and formats it as text for the console or as JSON for use by other tools";

static const struct option OPTIONS[14] =
{
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"file", required_argument, 0, 'f'},
    {"version", no_argument, 0, 'V'},
    {"outputdir", required_argument, 0, 'o'},
    {"list", no_argument, 0, 'l'},
    {"timestamps", no_argument, 0, 'T'},
    {"resolution", no_argument, 0, 'R'},
    {"no-error-bars", no_argument, 0, 'e'},
    {"by-name", required_argument, 0, 'n'},
    {"promise", optional_argument, 0, 'p'},
    {"uri", required_argument, 0, 'u'},
    {"override_dir", required_argument, 0, 'd'},
    {NULL, 0, 0, '\0'}
};

static const char *HINTS[14] =
{
    "Print the help message",
    "Output the version of the software",
    "Specify an alternative input file than the default",
    "Print version string for software",
    "Set output directory for printing graph data",
    "List available measurements",
    "Add a time stamp to directory name for graph file data",
    "Print graph data in high resolution",
    "Do not add error bars to the printed graphs",
    "Report only on the named measurement",
    "Report on agent promises",
    "Report on URI",
    "Override input master directory",
    NULL
};

/*****************************************************************************/

int main(int argc, char *argv[])
{
 GenericAgentConfig *config = CheckOpts(argc, argv);

 EvalContext *ctx = EvalContextNew();
 strcpy(CFWORKDIR, OVERRIDE);
 GenericAgentConfigApply(ctx, config);
 GenericAgentDiscoverContext(ctx, config);
 Policy *policy = LoadPolicy(ctx, config);
  
 ThisAgentInit();
 KeepReportsPromises();
 
 PolicyDestroy(policy);
 GenericAgentFinalize(ctx, config);
 return 0;
}

/*****************************************************************************/
/* Level 1                                                                   */
/*****************************************************************************/

static GenericAgentConfig *CheckOpts(int argc, char **argv)
{
 extern char *optarg;
 int optindex = 0;
 int c;
 GenericAgentConfig *config = GenericAgentConfigNewDefault(AGENT_TYPE_REPORTER);
 
 while ((c = getopt_long(argc, argv, "MVf:lR:o:n:p:u:d:", OPTIONS, &optindex)) != EOF)
    {
    switch ((char) c)
       {
       case 'f':
           GenericAgentConfigSetInputFile(config, GetInputDir(), optarg);
           MINUSF = true;
           break;
           
       case 'V':
       {
       Writer *w = FileWriter(stdout);
       GenericAgentWriteVersion(w);
       FileWriterDetach(w);
       }
       exit(EXIT_SUCCESS);
       
       case 'h':
           {
           Writer *w = FileWriter(stdout);
           GenericAgentWriteHelp(w, "cgn-report", OPTIONS, HINTS, true);
           FileWriterDetach(w);
           }   
         exit(EXIT_SUCCESS);
           
       case 'M':
           {
           Writer *out = FileWriter(stdout);
           ManPageWrite(out, "cgn-report", time(NULL),
                        CF_REPORT_SHORT_DESCRIPTION,
                        CF_REPORT_MANPAGE_LONG_DESCRIPTION,
                        OPTIONS, HINTS,
                        true);
           FileWriterDetach(out);
           exit(EXIT_SUCCESS);
           }   
       case 'o':
           strcpy(OUTPUTDIR, optarg);
           Log(LOG_LEVEL_VERBOSE, "Setting output directory to %s\n", OUTPUTDIR);
           break;
           
       case 'l':
           LISTALL = true;
           break;
           
       case 'R':
           HIRES = true;
           break;
           
       case 'n':
           strncpy(BYNAME, optarg, CF_BUFSIZE-1);
           break;

       case 'p':
           strncpy(BYPROMISE, optarg, CF_BUFSIZE-1);
           break;

       case 'N':
           NOWOPT = true;
           break;

       case 'u':
           strncpy(URI, optarg, CF_BUFSIZE-1);
           break;

       case 'd':
           snprintf(OVERRIDE,CF_BUFSIZE-1,"CFENGINE_TEST_OVERRIDE_WORKDIR=%s", optarg);
           putenv(OVERRIDE);
           break;
           
       default:
       {
       Writer *w = FileWriter(stdout);
       GenericAgentWriteHelp(w, "cgn-report", OPTIONS, HINTS, true);
       FileWriterDetach(w);
       }
       exit(EXIT_FAILURE);
       }
    }
 
 if (argv[optind] != NULL)
    {
    Log(LOG_LEVEL_ERR, "Unexpected argument with no preceding option: %s\n", argv[optind]);
    }
 
 return config;
}

/*****************************************************************************/

static void ThisAgentInit(void)
{
 time_t now;

 LogSetGlobalLevel(LOG_LEVEL_VERBOSE);
 
 if (strlen(OUTPUTDIR) == 0)
    {
    if (TIMESTAMPS)
       {
       if ((now = time((time_t *) NULL)) == -1)
          {
          Log(LOG_LEVEL_VERBOSE,"Couldn't read system clock\n");
          }
       
       snprintf(OUTPUTDIR, CF_BUFSIZE, "cgn-reports-%s-%s", CanonifyName(VFQNAME), cf_ctime(&now));
       }
    else
       {
       snprintf(OUTPUTDIR, CF_BUFSIZE, "cgn-reports-%s", CanonifyName(VFQNAME));
       }
    }

 umask(077);   
}

/*****************************************************************************/

static void KeepReportsPromises()
{
 char buf[CF_BUFSIZE];

 if (strlen(URI) > 0)
    {
    ReadAverages(false);
    HandleURI(URI);
    }
 else if (LISTALL)
    {
    ReadAverages(false);
    ListAll();
    }
 else if (strlen(BYNAME) > 0)
    {
    OutputSingleMeasure(BYNAME);
    }
 else if (strlen(BYPROMISE) > 0)
    {
    OutputPromises(BYPROMISE);
    }
 else
    {
    Banner("cgn-report");
    
    printf(" -> Output directory is set to %s/%s\n", getcwd(buf,CF_BUFSIZE),OUTPUTDIR);
    printf(" -> Current Time key is %s", GenTimeKey(time(NULL)));
    
    if (mkdir(OUTPUTDIR, 0755) == -1)
       {
       Log(LOG_LEVEL_VERBOSE, " -> Writing to existing directory\n");
       }
    
    if (chdir(OUTPUTDIR))
       {
       Log(LOG_LEVEL_ERR," !! Could not set the working directory");
       exit(0);
       }

    // monitor promises
    
    ReadAverages(true);
    
    Banner(" Weekly state monitoring promises (min - max) ranges");
    SummarizeAverages();
    
    Banner(" Single value monitoring promises");
    WriteNetPromises();
    Banner(" Write file dump");
    WriteGraphFiles();
    WriteHistograms();

    // Agent promises
    
    WriteAgentPromises();
    }
}

/*********************************************************************/

static void OutputSingleMeasure(char *name)
{
 if (!OutputWeekly(name))
    {
    OutputSingle(name);
    }
}

/*********************************************************************/

static void OutputPromises(char *name)
{
 CF_DB *dbp;
 CF_DBC *dbcp;
 char eventname[CF_MAXVARSIZE];
 Event entry;
 char *key;
 void *stored;
 int ksize, vsize;

 pcre *rx = CompileRegex(name);
 
 if (!rx)
    {
    Log(LOG_LEVEL_ERR, "Bad regular expression %s", name);
    return;
    }

 if (!OpenDB(&dbp, dbid_promises))
    {
    return;
    }
 
 if (!NewDBCursor(dbp, &dbcp))
    {
    CloseDB(dbp);
    return;
    }

 memset(&entry, 0, sizeof(entry));
 JsonElement *json = JsonObjectCreate(1);
 JsonElement *jsonarray = JsonArrayCreate(200);

 while (NextDB(dbcp, &key, &ksize, &stored, &vsize))
    {
    if (stored != NULL)
       {
       if (sizeof(entry) < vsize)
          {
          Log(LOG_LEVEL_ERR, "Invalid entry in measurements database. Expected size: %zu, actual size: %d", sizeof(entry), vsize);
          continue;
          }
       
       strcpy(eventname, (char *) key);
       memcpy(&entry, stored, MIN(vsize, sizeof(entry)));

       if (StringMatchWithPrecompiledRegex(rx, eventname, 0, 0))
          {
          char prefix[CF_BUFSIZE], handle[CF_BUFSIZE];
          prefix[0] = handle[0] = '\0';
          sscanf(eventname, "%[^,],%s", prefix, handle);
          
          JsonElement *jsondata = JsonObjectCreate(3);         
          JsonObjectAppendString(jsondata, "namespacepath", prefix);
          JsonObjectAppendString(jsondata, "handle", handle);
          JsonObjectAppendReal(jsondata, "q", entry.Q.q);
          JsonObjectAppendReal(jsondata, "expect", entry.Q.expect);
          JsonObjectAppendReal(jsondata, "var", entry.Q.var);
          JsonObjectAppendInteger(jsondata, "utc_last_checked", entry.t);
          
          if (entry.Q.q == 0) // promise not kept
             {
             char filename[CF_BUFSIZE], reason[CF_BUFSIZE];
             FILE *fp;
             
             snprintf(filename, CF_BUFSIZE, "%s/outputs/%s", CFWORKDIR, CanonifyName(eventname));
             
             if ((fp = fopen(filename, "r")) != NULL)
                {
                fgets(reason, CF_BUFSIZE-1, fp);
                JsonObjectAppendString(jsondata, "fault_reason", reason);
                fclose(fp);
                }
             }
          
          JsonArrayAppendElement(jsonarray, jsondata);
          }
       }
    }

 JsonObjectAppendArray(json, "compliance", jsonarray);
 Writer *writer = FileWriter(stdout);
 JsonWrite(writer, json, 0);
 WriterClose(writer);
 JsonDestroy(json);          
 pcre_free(rx);
 DeleteDBCursor(dbcp);
 CloseDB(dbp);
}

/*********************************************************************/

static void ListAll()
{
 CF_DB *dbp;
 CF_DBC *dbcp;
 char eventname[CF_MAXVARSIZE];
 Event entry;
 char *key;
 void *stored;
 int ksize, vsize, i;
 
 JsonElement *json = JsonObjectCreate(1);
 JsonElement *jsonarray = JsonArrayCreate(200);

 // Weekly

 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    char desc[CF_BUFSIZE];
    
    GetObservable(i, eventname, desc);

    if ((MAX.Q[i].expect != 0) && (strcmp(eventname,"spare") != 0))
       {
       JsonArrayAppendString(jsonarray, eventname);  
       }
    }

 // Singletons
  
 if (!OpenDB(&dbp, dbid_measure))
    {
    return;
    }
 
 if (!NewDBCursor(dbp, &dbcp))
    {
    Log(LOG_LEVEL_INFO, "Unable to scan class db");
    CloseDB(dbp);
    return;
    }
 
 memset(&entry, 0, sizeof(entry));

 while (NextDB(dbcp, &key, &ksize, &stored, &vsize))
    {
    strcpy(eventname, (char *) key);
    memcpy(&entry, stored, MIN(vsize, sizeof(entry)));
    JsonArrayAppendString(jsonarray, eventname);
    }

 DeleteDBCursor(dbcp);
 CloseDB(dbp);

 JsonObjectAppendArray(json, "measurements", jsonarray);

 Writer *writer = FileWriter(stdout);
 JsonWrite(writer, json, 0);
 WriterClose(writer);
 JsonDestroy(json); 
}


/*********************************************************************/

static bool OutputWeekly(char *cname)   
{
 int slot;
 char name[CF_BUFSIZE], desc[CF_BUFSIZE];
 
 for (slot = 0; slot < CF_OBSERVABLES; slot++)
    {
    GetObservable(slot, name, desc);
    if (strcmp(cname,name) == 0)
       {
       OutputWeeklyJSON(slot, name, desc);
       return true;
       }
    }

 return false;
}

/*********************************************************************/

static void OutputSingle(char *name)   
{
 CF_DB *dbp;
 CF_DBC *dbcp;
 char eventname[CF_MAXVARSIZE];
 Event entry;
 char *key;
 void *stored;
 int ksize, vsize;
 
 if (!OpenDB(&dbp, dbid_measure))
    {
    return;
    }
 
 if (!NewDBCursor(dbp, &dbcp))
    {
    Log(LOG_LEVEL_INFO, "Unable to scan class db");
    CloseDB(dbp);
    return;
    }
 
 memset(&entry, 0, sizeof(entry));

 while (NextDB(dbcp, &key, &ksize, &stored, &vsize))
    {
    if (stored != NULL)
       {
       if (sizeof(entry) < vsize)
          {
          continue;
          }
       
       strcpy(eventname, (char *) key);
       memcpy(&entry, stored, MIN(vsize, sizeof(entry)));

       if (strcmp(name, eventname) == 0)
          {
          JsonElement *json = JsonObjectCreate(6);         
          JsonObjectAppendString(json, "type", "singleton");
          JsonObjectAppendString(json, "name", name);
          JsonObjectAppendReal(json, "q", entry.Q.q);
          JsonObjectAppendReal(json, "expect", entry.Q.expect);
          JsonObjectAppendReal(json, "var", entry.Q.var);

          Writer *writer = FileWriter(stdout);
          JsonWrite(writer, json, 0);
          WriterClose(writer);
          JsonDestroy(json);
          }
       }
    }
 
 DeleteDBCursor(dbcp);
 CloseDB(dbp);
}

/*********************************************************************/

static void OutputWeeklyJSON(int slot, char *name, char *desc)   
{
 Averages entry;
 char timekey[CF_MAXVARSIZE];
 time_t now = time(NULL);
 CF_DB *dbp;
 int count = 0;

 if (slot > CF_OBSERVABLES)
    {
    return;
    }
 
 if (!OpenDB(&dbp, dbid_observations))
    {
    return;
    }

 JsonElement *json = JsonObjectCreate(5);         
 JsonObjectAppendString(json, "type", "weekly");
 JsonObjectAppendString(json, "name", name);
 JsonObjectAppendString(json, "description", desc);
 JsonObjectAppendInteger(json, "dataslot", slot);

 JsonElement *jsonarray = JsonArrayCreate(200);

 for (now = CF_MONDAY_MORNING; now < CF_MONDAY_MORNING + SECONDS_PER_WEEK; now += CF_MEASURE_INTERVAL)
    {
    strcpy(timekey, GenTimeKey(now));
    
    if (ReadDB(dbp, timekey, &entry, sizeof(Averages)))
       {
       JsonElement *jsondata = JsonObjectCreate(3);         
       JsonObjectAppendInteger(jsondata, "timeslot", count++);
       JsonObjectAppendString(jsondata, "timekey", timekey);
       JsonObjectAppendReal(jsondata, "q", entry.Q[slot].q);
       JsonObjectAppendReal(jsondata, "expect", entry.Q[slot].expect);
       JsonObjectAppendReal(jsondata, "var", entry.Q[slot].var);
       JsonArrayAppendElement(jsonarray, jsondata);
       }
    else
       {
       count++;
       }
    }

 JsonObjectAppendArray(json, "timeseries", jsonarray);

 Writer *writer = FileWriter(stdout);
 JsonWrite(writer, json, 0);
 WriterClose(writer);
 JsonDestroy(json);          

 CloseDB(dbp);
 
}

/*********************************************************************/

static void ReadAverages(bool verbose)
{
 Averages entry;
 char timekey[CF_MAXVARSIZE];
 char interval[CF_MAXVARSIZE];
 time_t now = time(NULL);
 CF_DB *dbp;
 int count = 0, i;
 
 if (!OpenDB(&dbp, dbid_observations))
    {
    return;
    }

 strcpy(interval, GenTimeKey(now));
 
 if (verbose && ReadDB(dbp, "DATABASE_AGE", &AGE, sizeof(double)))
    {
    printf( " -> Database age is %.2f (weeks)\n", AGE / SECONDS_PER_WEEK * CF_MEASURE_INTERVAL);
    }

 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    MAX.Q[i].var = MAX.Q[i].expect = MAX.Q[i].q = -99999999.0;
    MIN.Q[i].var = MIN.Q[i].expect = MIN.Q[i].q = 99999999.0;
    FPE[i] = FPQ[i] = NULL;
    HAVE_DATA[i] = false;
    }
 
 for (now = CF_MONDAY_MORNING; now < CF_MONDAY_MORNING + SECONDS_PER_WEEK; now += CF_MEASURE_INTERVAL)
    {
    strcpy(timekey, GenTimeKey(now));
    
    if (ReadDB(dbp, timekey, &entry, sizeof(Averages)))
       {
       for (i = 0; i < CF_OBSERVABLES; i++)
          {
          if (entry.Q[i].expect > MAX.Q[i].expect)
             {
             MAX.Q[i].expect = entry.Q[i].expect;
             }
          
          if (entry.Q[i].q > MAX.Q[i].q)
             {
             MAX.Q[i].q = entry.Q[i].q;
             }
          
          if (entry.Q[i].var > MAX.Q[i].var)
             {
             MAX.Q[i].var = entry.Q[i].var;
             }
          
          if (entry.Q[i].expect < MIN.Q[i].expect)
             {
             MIN.Q[i].expect = entry.Q[i].expect;
             }
          
          if (entry.Q[i].q < MIN.Q[i].q)
             {
             MIN.Q[i].q = entry.Q[i].q;
             }
          
          if (entry.Q[i].var < MIN.Q[i].var)
             {
             MIN.Q[i].var = entry.Q[i].var;
             }
          }
       }

    if (verbose && (strcmp(timekey,interval) == 0))
       {
       printf(" -> Current time's weekly timeslot is [%d] = UTC(%s)\n", count, timekey);
       }

    count++;
    }

 CloseDB(dbp);
}

/*****************************************************************************/

static void SummarizeAverages()
{
 int i;
 char name[CF_BUFSIZE];

 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    char desc[CF_BUFSIZE];
    
    GetObservable(i, name, desc);

    if (MAX.Q[i].expect != 0 && strcmp(name,"spare") != 0)
       {
       printf("[%2d] %15s: ranges Samples(%.2lf -> %.2lf) Average(%.2lf -> %.2lf) +/- Deviation(%.2lf- %.2lf)\n", i, name, MIN.Q[i].q, MAX.Q[i].q, MIN.Q[i].expect, MAX.Q[i].expect, sqrt(MIN.Q[i].var),sqrt(MAX.Q[i].var));
       HAVE_DATA[i] = true;
       }
    }
 }

/*****************************************************************************/

static void WriteGraphFiles()
{
 int i,count = 0;
 Averages entry;
 char timekey[CF_MAXVARSIZE];
 time_t now = time(NULL);
 CF_DB *dbp;
 
 if (!OpenDB(&dbp, dbid_observations))
    {
    return;
    }
 
 OpenFiles();
     
 for (now = CF_MONDAY_MORNING; now < CF_MONDAY_MORNING + SECONDS_PER_WEEK; now += CF_MEASURE_INTERVAL)
    {
    memset(&entry, 0, sizeof(entry));
    strcpy(timekey, GenTimeKey(now));   

    if (ReadDB(dbp, timekey, &entry, sizeof(Averages)))
       {
       }
    
    for (i = 0; i < CF_OBSERVABLES; i++)
       {
       if (HAVE_DATA[i])
          {
          fprintf(FPE[i], "%d %.4lf %.4lf\n", count, entry.Q[i].expect, sqrt(entry.Q[i].var));
          /* Use same scaling for Q so graphs can be merged */
          fprintf(FPQ[i], "%d %.4lf 0.0\n", count, entry.Q[i].q);
          }
       }

    count++;
    }   

 CloseDB(dbp);
 CloseFiles();
}

/*****************************************************************************/

static void WriteNetPromises()
{
 CF_DB *dbp;
 CF_DBC *dbcp;
 char eventname[CF_MAXVARSIZE];
 Event entry;
 char *key;
 void *stored;
 int ksize, vsize, count = 1;
 
 if (!OpenDB(&dbp, dbid_measure))
    {
    return;
    }
 
 if (!NewDBCursor(dbp, &dbcp))
    {
    CloseDB(dbp);
    return;
    }
 
 memset(&entry, 0, sizeof(entry));

 while (NextDB(dbcp, &key, &ksize, &stored, &vsize))
    {
    if (stored != NULL)
       {
       if (sizeof(entry) < vsize)
          {
          Log(LOG_LEVEL_ERR, "Invalid entry in measurements database. Expected size: %zu, actual size: %d", sizeof(entry), vsize);
          continue;
          }
       
       strcpy(eventname, (char *) key);
       memcpy(&entry, stored, MIN(vsize, sizeof(entry)));
       
       printf("[%d] %s q= %.2lf Av(%.2lf#%.2lf)\n", count++, eventname, entry.Q.q, entry.Q.expect,sqrt(entry.Q.var));
       }
    }
 
 DeleteDBCursor(dbcp);
 CloseDB(dbp);

}

/*****************************************************************************/

static void WriteHistograms()
{
 FILE *fp;
 int i, j, k, day, position;
 double maxval[CF_OBSERVABLES];
  char filename[CF_BUFSIZE];
 
 snprintf(filename, CF_BUFSIZE, "%s/state/histograms", CFWORKDIR);
 
 if ((fp = fopen(filename, "r")) == NULL)
    {
    Log(LOG_LEVEL_VERBOSE, "Unable to load histogram data. (fopen: %s)", GetErrorStr());
    return;
    }
 
 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    maxval[i] = 1.0;
    }
 
 for (position = 0; position < CF_GRAINS; position++)
    {
    if (fscanf(fp, "%d ", &position) != 1)
       {
       Log(LOG_LEVEL_ERR, "Format error in histogram file '%s' - aborting", filename);
       break;
       }
    
    for (i = 0; i < CF_OBSERVABLES; i++)
       {
       for (day = 0; day < 7; day++)
          {
          if (fscanf(fp, "%lf ", &(HISTOGRAM[i][day][position])) != 1)
             {
             Log(LOG_LEVEL_VERBOSE, "Format error in histogram file '%s'. (fscanf: %s)", filename, GetErrorStr());
             HISTOGRAM[i][day][position] = 0;
             }
          
          if (HISTOGRAM[i][day][position] < 0)
             {
             HISTOGRAM[i][day][position] = 0;
             }
          
          if (HISTOGRAM[i][day][position] > maxval[i])
             {
             maxval[i] = HISTOGRAM[i][day][position];
             }
          }
       }
    }
 
 fclose(fp);

 if (!HIRES)
    {
    /* Smooth daily and weekly histograms */
    for (k = 1; k < CF_GRAINS - 1; k++)
       {
       for (j = 0; j < CF_OBSERVABLES; j++)
          {
          for (i = 0; i < 7; i++)
             {
             SMOOTHHISTOGRAM[j][i][k] = (HISTOGRAM[j][i][k - 1] + HISTOGRAM[j][i][k] + HISTOGRAM[j][i][k + 1]) / 3.0;
             }
          }
       }
    }
 else
    {
    for (k = 1; k < CF_GRAINS - 1; k++)
       {
       for (j = 0; j < CF_OBSERVABLES; j++)
          {
          for (i = 0; i < 7; i++)
             {
             SMOOTHHISTOGRAM[j][i][k] = (double) HISTOGRAM[j][i][k];
             }
          }
       }
    }
 
 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    char desc[CF_BUFSIZE];
    char name[CF_BUFSIZE];
    
    GetObservable(i, name, desc);
    snprintf(filename, CF_BUFSIZE, "%s.distr", name);
    
    if ((FPQ[i] = fopen(filename, "w")) == NULL)
       {
       Log(LOG_LEVEL_INFO, "Couldn't write %s", name);
       return;
       }
    }
 
/* Plot daily and weekly histograms */
 for (k = 0; k < CF_GRAINS; k++)
    {
    int a;
    double weekly[CF_OBSERVABLES][7];
    
    for (j = 0; j < CF_OBSERVABLES; j++)
       {
       for (i = 0; i < 7; i++)
          {
          weekly[j][k] += SMOOTHHISTOGRAM[j][i][k];
          }
       }
    
    for (a = 0; a < CF_OBSERVABLES; a++)
       {
       fprintf(FPQ[a], "%d %.2lf\n", k, weekly[a][k]);
       }
    }
 
 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    fclose(FPQ[i]);
    }
}

/********************************************************************************/

static void OpenFiles()
{
 int i;
 char filename[CF_BUFSIZE], name[CF_MAXVARSIZE];
 
 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    char desc[CF_BUFSIZE];
    
    if (HAVE_DATA[i])
       {
       GetObservable(i, name, desc);
       snprintf(filename, CF_BUFSIZE, "%s.avg", name);

       Log(LOG_LEVEL_VERBOSE, " -> Writing average data to %s/%s\n", OUTPUTDIR, filename);
       
       if ((FPE[i] = fopen(filename, "w")) == NULL)
          {
          Log(LOG_LEVEL_ERR, " !! File %s could not be opened for writing\n", filename);
          return;
          }
       
       snprintf(filename, CF_BUFSIZE, "%s.q", name);
       
       if ((FPQ[i] = fopen(filename, "w")) == NULL)
          {
          Log(LOG_LEVEL_ERR, " !! File %s could not be opened for writing\n", filename);
          return;
          }
       }
    }
}

/*********************************************************************/

static void CloseFiles()
{
 int i;
 
 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    if (HAVE_DATA[i])
       {
       fclose(FPE[i]);
       fclose(FPQ[i]);
       }
    }
}

/*******************************************************************/

char *cf_ctime(const time_t *timep)
{
 static char buf[26];
 return cf_strtimestamp_local(*timep, buf);
}

/*******************************************************************/

char *cf_strtimestamp_local(const time_t time, char *buf)
{
 struct tm tm;
 
 if (localtime_r(&time, &tm) == NULL)
    {
    Log(LOG_LEVEL_ERR, "Unable to parse passed timestamp");
    return NULL;
    }

 return cf_format_strtimestamp(&tm, buf);
}


/*******************************************************************/

static char *cf_format_strtimestamp(struct tm *tm, char *buf)
{
 /* Security checks */
 if ((tm->tm_year < -2899) || (tm->tm_year > 8099))
    {
    Log(LOG_LEVEL_ERR, "Unable to format timestamp: passed year is out of range: %d", tm->tm_year + 1900);
    return NULL;
    }
 
/* There is no easy way to replicate ctime output by using strftime */
 
 if (snprintf(buf, 26, "%3.3s %3.3s %2d %02d:%02d:%02d %04d",
              DAY_TEXT[tm->tm_wday ? (tm->tm_wday - 1) : 6], MONTH_TEXT[tm->tm_mon],
              tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_year + 1900) >= 26)
    {
    Log(LOG_LEVEL_ERR, "Unable to format timestamp: passed values are out of range");
    return NULL;
    }
 
 return buf;
}

/*****************************************************************************/

static void WriteAgentPromises()
{
 CF_DB *dbp;
 CF_DBC *dbcp;
 char eventname[CF_MAXVARSIZE];
 Event entry;
 char *key;
 void *stored;
 int ksize, vsize, count = 1;

 Banner(" Agent promise status");
 
 if (!OpenDB(&dbp, dbid_promises))
    {
    return;
    }
 
 if (!NewDBCursor(dbp, &dbcp))
    {
    CloseDB(dbp);
    return;
    }
 
 memset(&entry, 0, sizeof(entry));

 while (NextDB(dbcp, &key, &ksize, &stored, &vsize))
    {
    if (stored != NULL)
       {
       if (sizeof(entry) < vsize)
          {
          Log(LOG_LEVEL_ERR, "Invalid entry in measurements database. Expected size: %zu, actual size: %d", sizeof(entry), vsize);
          continue;
          }
       
       strcpy(eventname, (char *) key);
       memcpy(&entry, stored, MIN(vsize, sizeof(entry)));
       printf("[%d] %s q= %.2lf Av(%.2lf#%.2lf) last checked at %s\n", count++, eventname, entry.Q.q, entry.Q.expect,sqrt(entry.Q.var), cf_ctime(&(entry.t)));
          if (entry.Q.q == 0) // promise not kept
             {
             char filename[CF_BUFSIZE], reason[CF_BUFSIZE];
             FILE *fp;
             
             snprintf(filename, CF_BUFSIZE, "%s/outputs/%s", CFWORKDIR, CanonifyName(eventname));
             
             if ((fp = fopen(filename, "r")) != NULL)
                {
                fgets(reason, CF_BUFSIZE-1, fp);
                printf("    Reason for last failure: %s\n", reason);
                fclose(fp);
                }
             }

       }
    }

 DeleteDBCursor(dbcp);
 CloseDB(dbp);
}


/**********************************************************************************************/
/* This part is for the REST API - if this gets productized, should design this tool better   */
/**********************************************************************************************/

static void HandleURI(char *uri)

{
 char prefix[CF_MAXVARSIZE] = {0};
 char agent[CF_MAXVARSIZE] = {0};
 char handle[CF_MAXVARSIZE] = {0};
 char element[CF_MAXVARSIZE] = {0};

 sscanf(URI, "/%[^/]/%[^/]/%[^/]/%s",prefix,agent,handle,element);

 if (strcmp(prefix,"cgn") != 0)
    {
    return;
    }

 if (strcmp(agent,"agent") == 0)
    {
    if (handle[0] != '\0')
       {
       HandleAgentHandle(handle,element);
       }
    else
       {
       ListAgentHandles();
       }
    }

 if (strcmp(agent,"monitor") == 0)
    {
    if (handle[0] != '\0')
       {
       HandleMeasurementHandle(handle,element);
       }
    else
       {
       ListMeasurementHandles();
       }
    }


}

/**********************************************************************************************/

static void HandleAgentHandle(char *handle, char *element)

{
 printf("HANDLE handle\n");

 printf("JSON AGENT/%s/%s\n", handle,element);
}


/**********************************************************************************************/

static void HandleMeasurementHandle(char *handle, char *element)

{
 printf("HANDLE mon handle\n");
        
 printf("JSON MONITOR/%s/%s\n", handle,element);
}


/**********************************************************************************************/

static void ListAgentHandles()

{
 CF_DB *dbp;
 CF_DBC *dbcp;
 char eventname[CF_MAXVARSIZE];
 Event entry;
 char *key;
 void *stored;
 int ksize, vsize;

 if (!RO_OpenDB(&dbp, dbid_promises))
    {
    printf("Failed to open\n");
    perror("open");
    return;
    }

 if (!NewDBCursor(dbp, &dbcp))
    {
    CloseDB(dbp);
    return;
    }

 memset(&entry, 0, sizeof(entry));
 JsonElement *json = JsonObjectCreate(1);
 JsonElement *jsonarray = JsonArrayCreate(200);

 while (NextDB(dbcp, &key, &ksize, &stored, &vsize))
    {
    if (stored != NULL)
       {
       if (sizeof(entry) < vsize)
          {
          Log(LOG_LEVEL_ERR, "Invalid entry in measurements database. Expected size: %zu, actual size: %d", sizeof(entry), vsize);
          continue;
          }
       
       strcpy(eventname, (char *) key);          
       JsonArrayAppendString(jsonarray, eventname);
       }
    }

 JsonObjectAppendArray(json, "promises", jsonarray);
 Writer *writer = FileWriter(stdout);
 JsonWrite(writer, json, 0);
 WriterClose(writer);
 JsonDestroy(json);          
 DeleteDBCursor(dbcp);
 RO_CloseDB(dbp);
}


/**********************************************************************************************/

static void ListMeasurementHandles()

{
 ListAll();
}


/**********************************************************************************************/


/* EOF */
