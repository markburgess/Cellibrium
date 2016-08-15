
/*

 This file is (C) Cfengine AS. See COSL LICENSE for details.

*/

#include <cf3.defs.h>
#include <cf3.extern.h>
#include <known_dirs.h>
#include <eval_context.h>
#include <promises.h>
#include <probes.h>
#include <files_lib.h>
#include <files_names.h>
#include <files_interfaces.h>
#include <vars.h>
#include <item_lib.h>
#include <conversion.h>
#include <scope.h>
#include <matching.h>
#include <instrumentation.h>
#include <pipes.h>
#include <locks.h>
#include <string_lib.h>
#include <exec_tools.h>
#include <unix.h>
#include <file_lib.h>
#include <history.h>
#include <monitoring.h>
#include <ornaments.h>

typedef struct
{
    char *name;
    char *description;
    char *units;
    double expected_minimum;
    double expected_maximum;
    bool consolidable;
} MonitoringSlot;

/* Constants */

static const char *UNITS[CF_OBSERVABLES] =
{
    "average users per 2.5 mins",
    "processes",
    "processes",
    "percent",
    "jobs",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "packets",
    "packets",
    "packets",
    "packets",
    "packets",
    "packets",
    "packets",
    "packets",
    "packets",
    "packets",
    "packets",
    "packets",
    "packets",
    "packets",
    "entries",
    "entries",
    "entries",
    "entries",
    "Celcius",
    "Celcius",
    "Celcius",
    "Celcius",
    "percent",
    "percent",
    "percent",
    "percent",
    "percent",
    "packets",
    "packets",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",
    "connections",

};

static time_t slots_load_time = 0;
static MonitoringSlot *SLOTS[CF_OBSERVABLES - ob_spare];

/*****************************************************************************/

static void Mon_FreeSlot(MonitoringSlot *slot)
{
 if (slot)
    {
    free(slot->name);
    free(slot->description);
    free(slot->units);
    free(slot);
    }
}

/*****************************************************************************/

static MonitoringSlot *Mon_MakeSlot(const char *name, const char *description,
                                     const char *units,
                                     double expected_minimum, double expected_maximum, bool consolidable)
{
 MonitoringSlot *slot = xmalloc(sizeof(MonitoringSlot));
 
 slot->name = xstrdup(name);
 slot->description = xstrdup(description);
 slot->units = xstrdup(units);
 slot->expected_minimum = expected_minimum;
 slot->expected_maximum = expected_maximum;
 slot->consolidable = consolidable;
 return slot;
}

/*****************************************************************************/

static void Mon_LoadSlots(void)
{
 FILE *f;
 char filename[CF_BUFSIZE];
 int i;
 
 snprintf(filename, CF_BUFSIZE - 1, "%s%cstate%cts_key", CFWORKDIR, FILE_SEPARATOR, FILE_SEPARATOR);
 
 struct stat sb;
 
 if (stat(filename, &sb) != 0)
    {
    return;
    }
 
 if(sb.st_mtime <= slots_load_time)
    {
    return;
    }
 
 slots_load_time = sb.st_mtime;
 
 if ((f = fopen(filename, "r")) == NULL)
    {
    return;
    }
 
 for (i = 0; i < CF_OBSERVABLES; ++i)
    {
    if (i < ob_spare)
       {
       fscanf(f, "%*[^\n]\n");
       }
    else
       {
       char line[CF_MAXVARSIZE];
       
       char name[CF_MAXVARSIZE], desc[CF_MAXVARSIZE];
       char units[CF_MAXVARSIZE] = "unknown";
       double expected_min = 0.0;
       double expected_max = 100.0;
       int consolidable = true;
       
       if (fgets(line, CF_MAXVARSIZE, f) == NULL)
          {
          Log(LOG_LEVEL_ERR, "Error trying to read ts_key from file '%s'. (fgets: %s)", filename, GetErrorStr());
          continue;
          }
       
       int fields = sscanf(line, "%*d,%1023[^,],%1023[^,],%1023[^,],%lf,%lf,%d",
                           name, desc, units, &expected_min, &expected_max, &consolidable);
       
       if (fields == 2)
          {
          /* Old-style ts_key with name and description */
          }
       else if (fields == 6)
          {
          /* New-style ts_key with additional parameters */
          }
       else
          {
          Log(LOG_LEVEL_ERR, "Wrong line format in ts_key: %s", line);
          }
       
       if (strcmp(name, "spare") != 0)
          {
          Mon_FreeSlot(SLOTS[i - ob_spare]);
          SLOTS[i - ob_spare] = Mon_MakeSlot(name, desc, units, expected_min, expected_max, consolidable);
          }
       }
    }
 fclose(f);
}

/*****************************************************************************/

void Mon_DumpSlots(void)
{
#define MAX_KEY_FILE_SIZE 16384  /* usually around 4000, cannot grow much */

 char filename[CF_BUFSIZE];
 int i;
 
 snprintf(filename, CF_BUFSIZE - 1, "%s%cstate%cts_key", CFWORKDIR, FILE_SEPARATOR, FILE_SEPARATOR);
 
 char file_contents_new[MAX_KEY_FILE_SIZE] = {0};
 
 for (i = 0; i < CF_OBSERVABLES; i++)
    {
    char line[CF_MAXVARSIZE];
    
    if (MonHasSlot(i))
       {
       snprintf(line, sizeof(line), "%d,%s,%s,%s,%.3lf,%.3lf,%d\n",
                i,
                NULLStringToEmpty((char*)MonGetSlotName(i)),
                NULLStringToEmpty((char*)MonGetSlotDescription(i)),
                NULLStringToEmpty((char*)MonGetSlotUnits(i)),
                MonGetSlotExpectedMinimum(i), MonGetSlotExpectedMaximum(i), MonIsSlotConsolidable(i) ? 1 : 0);
       }
    else
       {
       snprintf(line, sizeof(line), "%d,spare,unused\n", i);
       }
    
    strlcat(file_contents_new, line, sizeof(file_contents_new));
    }
 
 bool contents_changed = true;
 
 Writer *w = FileRead(filename, MAX_KEY_FILE_SIZE, NULL);
    if (w)
       {
       if(strcmp(StringWriterData(w), file_contents_new) == 0)
          {
          contents_changed = false;
          }
       WriterClose(w);
       }
    
    if(contents_changed)
       {
       Log(LOG_LEVEL_VERBOSE, "Updating %s with new slot information", filename);
       
       if(!FileWriteOver(filename, file_contents_new))
          {
          Log(LOG_LEVEL_ERR, "Mon_DumpSlots: Could not write file '%s'. (FileWriteOver: %s)", filename,
              GetErrorStr());
          }
       }
    
    chmod(filename, 0600);
}

/*****************************************************************************/

void GetObservable(int i, char *name, char *desc)
{
 Mon_LoadSlots();
 
 if (i < ob_spare)
    {
    strncpy(name, OBS[i][0], CF_MAXVARSIZE - 1);
    strncpy(desc, OBS[i][1], CF_MAXVARSIZE - 1);
    }
 else
    {
    if (SLOTS[i - ob_spare])
       {
       strncpy(name, SLOTS[i - ob_spare]->name, CF_MAXVARSIZE - 1);
       strncpy(desc, SLOTS[i - ob_spare]->description, CF_MAXVARSIZE - 1);
       }
    else
       {
       strncpy(name, OBS[i][0], CF_MAXVARSIZE - 1);
       strncpy(desc, OBS[i][1], CF_MAXVARSIZE - 1);
       }
    }
}

/*****************************************************************************/

void SetMeasurementPromises(Item **classlist)
{
 CF_DB *dbp;
 CF_DBC *dbcp;
 char eventname[CF_MAXVARSIZE], assignment[CF_BUFSIZE];
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
          Log(LOG_LEVEL_ERR, "Invalid entry in measurements database. Expected size: %zu, actual size: %d", sizeof(entry), vsize);
          continue;
          }
       
       strcpy(eventname, (char *) key);
       memcpy(&entry, stored, MIN(vsize, sizeof(entry)));
       
       Log(LOG_LEVEL_VERBOSE, "[%d] %s q= %.2lf var= %.2lf ex= %.2lf -> (%.2lf#%.2lf)", count++, eventname, entry.Q.q, entry.Q.var, entry.Q.expect, entry.Q.expect,sqrt(entry.Q.var));
              
       if ((int) (entry.Q.q * 10) % 10 == 0)
          {
          snprintf(assignment, CF_BUFSIZE - 1, "value_%s=%.0lf", eventname, entry.Q.q);
          }
       else
          {
          snprintf(assignment, CF_BUFSIZE - 1, "value_%s=%.2lf", eventname, entry.Q.q);
          }
       Log(LOG_LEVEL_VERBOSE,"  + %s", assignment);
       AppendItem(classlist, assignment, NULL);

       snprintf(assignment, CF_BUFSIZE - 1, "av_%s=%.2lf", eventname, entry.Q.expect);
       Log(LOG_LEVEL_VERBOSE,"  + %s", assignment);
       AppendItem(classlist, assignment, NULL);
       
       snprintf(assignment, CF_BUFSIZE - 1, "dev_%s=%.2lf", eventname, sqrt(entry.Q.var));
       Log(LOG_LEVEL_VERBOSE,"  + %s", assignment);
       AppendItem(classlist, assignment, NULL);
       }
    }
 
 DeleteDBCursor(dbcp);
 CloseDB(dbp);
}

/*****************************************************************************/

void LoadSlowlyVaryingObservations(EvalContext *ctx) 
{
 CF_DB *dbp;
 CF_DBC *dbcp;
 char *key;
 void *stored;
 int ksize, vsize;
 
 if (!OpenDB(&dbp, dbid_static))
    {
    return;
    }
 
/* Acquire a cursor for the database. */
 
 if (!NewDBCursor(dbp, &dbcp))
    {
    Log(LOG_LEVEL_INFO, "Unable to scan class db");
    CloseDB(dbp);
    return;
    }
 
 while (NextDB(dbcp, &key, &ksize, &stored, &vsize))
    {
    char buf[CF_MAXVARSIZE], lval[CF_MAXVARSIZE], rval[CF_BUFSIZE];
    DataType type;
    Rlist *list = NULL;
    
    strncpy(buf, key, CF_MAXVARSIZE - 1);
    
    int type_i;
    sscanf(buf, "%[^:]:%d", lval, &type_i);
    type = (DataType)type_i;
    
    if (stored != NULL)
       {
       strncpy(rval, stored, CF_BUFSIZE - 1);
       
       switch (type)
          {
          case CF_DATA_TYPE_STRING:
          case CF_DATA_TYPE_INT:
          case CF_DATA_TYPE_REAL:
              EvalContextVariablePutSpecial(ctx, SPECIAL_SCOPE_MON, lval, rval, type, "monitoring,source=observation");
              break;
              
          case CF_DATA_TYPE_STRING_LIST:
              list = RlistFromSplitString(rval, ',');
              EvalContextVariablePutSpecial(ctx, SPECIAL_SCOPE_MON, lval, list, CF_DATA_TYPE_STRING_LIST, "monitoring,source=observation");
              RlistDestroy(list);
              break;
              
          case CF_DATA_TYPE_COUNTER:
              EvalContextVariablePutSpecial(ctx, SPECIAL_SCOPE_MON, lval, rval, CF_DATA_TYPE_STRING, "monitoring,source=observation");
              break;
              
          default:
              Log(LOG_LEVEL_VERBOSE, "Unexpected value type reading from database: %d", (int) type);
          }
       }
    }
 
 DeleteDBCursor(dbcp);
 CloseDB(dbp);
}

/*****************************************************************************/
/* Level                                                                     */
/*****************************************************************************/

const int MonGetSlot(const char *name)
{
 int i;
 
 Mon_LoadSlots();

/* First try to find existing slot */
 for (i = 0; i < CF_OBSERVABLES - ob_spare; ++i)
    {
    if (SLOTS[i] && !strcmp(SLOTS[i]->name, name))
       {
       Log(LOG_LEVEL_VERBOSE, "[%d] slot selected for %s", i + ob_spare, name);
       return i + ob_spare;
       }
    }
 
/* Then find the spare one */
 for (i = 0; i < CF_OBSERVABLES - ob_spare; ++i)
    {
    if (!SLOTS[i])
       {
       Log(LOG_LEVEL_VERBOSE, "[%d] slot selected for %s", i + ob_spare, name);
       return i + ob_spare;
       }
    }
 
 Log(LOG_LEVEL_ERR,
     "Measurement slots are all in use - it is not helpful to measure too much, you can't usefully follow this many variables");
 
 return -1;
}
                                                                          
/*****************************************************************************/

int MonRegisterSlot(const char *name, const char *description, const char *units, double expected_minimum, double expected_maximum, bool consolidable)
{
 int slot = MonGetSlot(name);
 
 if (slot == -1)
    {
    return -1;
    }
 
 Mon_FreeSlot(SLOTS[slot - ob_spare]);
 SLOTS[slot - ob_spare] = Mon_MakeSlot(name, description, units, expected_minimum, expected_maximum, consolidable);
 Mon_DumpSlots();
 
 return slot;
}

/*****************************************************************************/

bool MonHasSlot(int idx)
{
 Mon_LoadSlots();
 return idx < ob_spare || SLOTS[idx - ob_spare];
}

/*****************************************************************************/

const char *MonGetSlotName(int idx)
{
 Mon_LoadSlots(); 
 return idx < ob_spare ? OBS[idx][0] : SLOTS[idx - ob_spare]->name;
}

/*****************************************************************************/

const char *MonGetSlotDescription(int idx)
{
 Mon_LoadSlots();
 return idx < ob_spare ? OBS[idx][1] : SLOTS[idx - ob_spare]->description;
}

/*****************************************************************************/

const char *MonGetSlotUnits(int idx)
{
 Mon_LoadSlots();
 return idx < ob_spare ? UNITS[idx] : SLOTS[idx - ob_spare]->units;
}

/*****************************************************************************/
// TODO: real expected minimum/maximum/consolidable for core slots

double MonGetSlotExpectedMinimum(int idx)
{
 Mon_LoadSlots();
 return idx < ob_spare ? 0.0f : SLOTS[idx - ob_spare]->expected_minimum;
}

/*****************************************************************************/

double MonGetSlotExpectedMaximum(int idx)
{
 Mon_LoadSlots();
 return idx < ob_spare ? 100.0f : SLOTS[idx - ob_spare]->expected_maximum;
}

/*****************************************************************************/

bool MonIsSlotConsolidable(int idx)
{
 Mon_LoadSlots();
 return idx < ob_spare ? true : SLOTS[idx - ob_spare]->consolidable;
}

/*****************************************************************************************/

void MonNamedEvent(const char *eventname, double value)
{
 Event ev_new, ev_old;
 time_t now = time(NULL);
 CF_DB *dbp;
 
 if (!OpenDB(&dbp, dbid_measure))
    {
    return;
    }
 
 ev_new.t = now;
 
 if (ReadDB(dbp, eventname, &ev_old, sizeof(ev_old)))
    {
    if (isnan(ev_old.Q.expect))
       {
       ev_old.Q.expect = value;
       }
    
    if (isnan(ev_old.Q.var))
       {
       ev_old.Q.var = 0;
       }
    
    ev_new.Q = QAverage(ev_old.Q, value, 0.7);
    }
 else
    {
    ev_new.Q = QDefinite(value);
    }
 
 Log(LOG_LEVEL_VERBOSE, "Wrote scalar named event \"%s\" = (%.2lf,%.2lf,%.2lf)", eventname, ev_new.Q.q,
     ev_new.Q.expect, sqrt(ev_new.Q.var));
 WriteDB(dbp, eventname, &ev_new, sizeof(ev_new));
 
 CloseDB(dbp);
}

/*****************************************************************************/
/* Level                                                                     */
/*****************************************************************************/

/*
 * This function returns beginning of last Monday relative to 'time'. If 'time'
 * is Monday, beginning of the same day is returned.
 */
time_t WeekBegin(time_t time)
{
 struct tm tm;
 
 gmtime_r(&time, &tm);
 
/* Move back in time to reach Monday. */
 
 time -= ((tm.tm_wday == 0 ? 6 : tm.tm_wday - 1) * SECONDS_PER_DAY);
 
/* Move to the beginning of day */
 
 time -= tm.tm_hour * SECONDS_PER_HOUR;
 time -= tm.tm_min * SECONDS_PER_MINUTE;
 time -= tm.tm_sec;
 
 return time;
}

/****************************************************************************/

time_t SubtractWeeks(time_t time, int weeks)
{
 return time - weeks * SECONDS_PER_WEEK;
}

/****************************************************************************/

time_t NextShift(time_t time)
{
 return time + SECONDS_PER_SHIFT;
}


/****************************************************************************/

/* Returns true if entry was found, false otherwise */
bool GetRecordForTime(CF_DB *db, time_t time, Averages *result)
{
 char timekey[CF_MAXVARSIZE]; 
 MakeTimekey(time, timekey);
 return ReadDB(db, timekey, result, sizeof(Averages));
}
