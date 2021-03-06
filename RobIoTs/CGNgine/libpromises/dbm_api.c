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

#include <file_lib.h>

#include <mutex.h>                                            /* ThreadLock */
#include <dbm_api.h>
#include <dbm_priv.h>
#include <dbm_migration.h>
#include <atexit.h>
#include <logging.h>
#include <misc_lib.h>


static int DBPathLock(const char *filename);
static void DBPathUnLock(int fd);
static void DBPathMoveBroken(const char *filename);

struct DBHandle_
{
    /* Filename of database file */
    char *filename;

    /* Actual database-specific data */
    DBPriv *priv;

    int refcount;

    /* This lock protects initialization of .priv element, and .refcount manipulation */
    pthread_mutex_t lock;
};

struct DBCursor_
{
    DBCursorPriv *cursor;
};

/******************************************************************************/

/*
 * This lock protects on-demand initialization of db_handles[i].lock and
 * db_handles[i].name.
 */
static pthread_mutex_t db_handles_lock = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP; /* GLOBAL_T */

static DBHandle db_handles[dbid_max] = { { 0 } }; /* GLOBAL_X */

static pthread_once_t db_shutdown_once = PTHREAD_ONCE_INIT; /* GLOBAL_T */

/******************************************************************************/

static const char *const DB_PATHS[] = {
    [dbid_classes] = "cf_classes",
    [dbid_variables] = "state/cf_variables",
    [dbid_performance] = "performance",
    [dbid_checksums] = "checksum_digests",
    [dbid_filestats] = "stats",
    [dbid_changes] = "state/cf_changes",
    [dbid_observations] = "state/cf_observations",
    [dbid_state] = "state/cf_state",
    [dbid_lastseen] = "cf_lastseen",
    [dbid_timescales] = "state/cf_timescales",
    [dbid_locks] = "state/cf_lock",
    [dbid_history] = "state/history",
    [dbid_measure] = "state/measures",
    [dbid_static] = "state/static",
    [dbid_scalars] = "state/pscalar",
    [dbid_processanomalies] = "state/process",
    [dbid_cache] = "cache",
    [dbid_license] = "nova_track",
    [dbid_value] = "nova_value",
    [dbid_agent_execution] = "nova_agent_execution",
    [dbid_bundles] = "bundles",
    [dbid_promises] = "promises",
};

/******************************************************************************/

char *DBIdToPath(const char *workdir, dbid id)
{
    assert(DB_PATHS[id] != NULL);

    char *filename;
    if (xasprintf(&filename, "%s/%s.%s",
                  workdir, DB_PATHS[id], DBPrivGetFileExtension()) == -1)
    {
        ProgrammingError("Unable to construct database filename for file %s", DB_PATHS[id]);
    }

    char *native_filename = MapNameCopy(filename);
    free(filename);

    return native_filename;
}

static DBHandle *DBHandleGet(int id)
{
    assert(id >= 0 && id < dbid_max);

    ThreadLock(&db_handles_lock);

    if (db_handles[id].filename == NULL)
    {
        db_handles[id].filename = DBIdToPath(CFWORKDIR, id);

        /* Initialize mutexes as error-checking ones. */
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
        pthread_mutex_init(&db_handles[id].lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    ThreadUnlock(&db_handles_lock);

    return &db_handles[id];
}

/**
 * @brief Wait for all users of all databases to close the DBs. Then acquire
 * the mutexes *AND KEEP THEM LOCKED* so that no background thread can open
 * any database. So make sure you exit soon...
 *
 * @warning This is usually register with atexit(), however you have to make
 * sure no other DB-cleaning exit hook was registered before, so that this is
 * called last.
 **/
void CloseAllDBExit()
{
    ThreadLock(&db_handles_lock);

    for (int i = 0; i < dbid_max; i++)
    {
        if (db_handles[i].filename)
        {
            /* Wait until all DB users are served, or a threshold is reached */
            int count = 0;
            ThreadLock(&db_handles[i].lock);
            while (db_handles[i].refcount > 0 && count < 1000)
            {
                ThreadUnlock(&db_handles[i].lock);

                struct timespec sleeptime = {
                    .tv_sec = 0,
                    .tv_nsec = 10000000                         /* 10 ms */
                };
                nanosleep(&sleeptime, NULL);
                count++;

                ThreadLock(&db_handles[i].lock);
            }
            /* Keep mutex locked. */

            /* If we exited because of timeout make sure we Log() it. */
            if (db_handles[i].refcount != 0)
            {
                Log(LOG_LEVEL_VERBOSE,
                    "Database %s refcount is still not zero (%d), forcing CloseDB()!",
                    db_handles[i].filename, db_handles[i].refcount);
                DBPrivCloseDB(db_handles[i].priv);
            }
            else /* TODO: can we clean this up unconditionally ? */
            {
                free(db_handles[i].filename);
                db_handles[i].filename = NULL;
            }
        }
    }
}

static void RegisterShutdownHandler(void)
{
    RegisterAtExitFunction(&CloseAllDBExit);
}

/**
 * Keeps track of the maximum number of concurrent transactions, which is
 * expected to be set by agents as they start up. If it is not set it will use
 * the existing value. If it is set, but the database cannot honor it, CFEngine
 * will warn.
 * @param max_txn Maximum number of concurrent transactions for a single
 *                database.
 */
void DBSetMaximumConcurrentTransactions(int max_txn)
{
    DBPrivSetMaximumConcurrentTransactions(max_txn);
}

bool OpenDB(DBHandle **dbp, dbid id)
{
    DBHandle *handle = DBHandleGet(id);

    ThreadLock(&handle->lock);

    if (handle->refcount == 0)
    {
        int lock_fd = DBPathLock(handle->filename);

        if(lock_fd != -1)
        {
            handle->priv = DBPrivOpenDB(handle->filename, id);

            if (handle->priv == DB_PRIV_DATABASE_BROKEN)
            {
                DBPathMoveBroken(handle->filename);
                handle->priv = DBPrivOpenDB(handle->filename, id);
                if (handle->priv == DB_PRIV_DATABASE_BROKEN)
                {
                    handle->priv = NULL;
                }
            }

            DBPathUnLock(lock_fd);
        }

        if (handle->priv)
        {
            if (!DBMigrate(handle, id))
            {
                DBPrivCloseDB(handle->priv);
                handle->priv = NULL;
            }
        }
    }

    if (handle->priv)
    {
        handle->refcount++;
        *dbp = handle;

        /* Only register shutdown handler if any database was opened
         * correctly. Otherwise this shutdown caller may be called too early,
         * and shutdown handler installed by the database library may end up
         * being called before CloseAllDB function */

        pthread_once(&db_shutdown_once, RegisterShutdownHandler);
    }
    else
    {
        *dbp = NULL;
    }

    ThreadUnlock(&handle->lock);

    return *dbp != NULL;
}



void CloseDB(DBHandle *handle)
{
    ThreadLock(&handle->lock);

    DBPrivCommit(handle->priv);

    if (handle->refcount < 1)
    {
        Log(LOG_LEVEL_VERBOSE, "Trying to close database %s which is not open", handle->filename);
    }
    else if (--handle->refcount == 0)
    {
        DBPrivCloseDB(handle->priv);
    }

    ThreadUnlock(&handle->lock);
}

/*****************************************************************************/

bool RO_OpenDB(DBHandle **dbp, dbid id)
{
    DBHandle *handle = DBHandleGet(id);
    handle->priv = DBPrivOpenDB(handle->filename, id);

    if (handle->priv)
    {
    *dbp = handle;
    }
    else
       {
       *dbp = NULL;
       }
    return *dbp != NULL;
}


void RO_CloseDB(DBHandle *handle)
{
DBPrivCloseDB(handle->priv);
}


/*****************************************************************************/

bool ReadComplexKeyDB(DBHandle *handle, const char *key, int key_size,
                      void *dest, int dest_size)
{
    return DBPrivRead(handle->priv, key, key_size, dest, dest_size);
}

bool WriteComplexKeyDB(DBHandle *handle, const char *key, int key_size,
                       const void *value, int value_size)
{
    return DBPrivWrite(handle->priv, key, key_size, value, value_size);
}

bool DeleteComplexKeyDB(DBHandle *handle, const char *key, int key_size)
{
    return DBPrivDelete(handle->priv, key, key_size);
}

bool ReadDB(DBHandle *handle, const char *key, void *dest, int destSz)
{
    return DBPrivRead(handle->priv, key, strlen(key) + 1, dest, destSz);
}

bool WriteDB(DBHandle *handle, const char *key, const void *src, int srcSz)
{
    return DBPrivWrite(handle->priv, key, strlen(key) + 1, src, srcSz);
}

bool HasKeyDB(DBHandle *handle, const char *key, int key_size)
{
    return DBPrivHasKey(handle->priv, key, key_size);
}

int ValueSizeDB(DBHandle *handle, const char *key, int key_size)
{
    return DBPrivGetValueSize(handle->priv, key, key_size);
}

bool DeleteDB(DBHandle *handle, const char *key)
{
    return DBPrivDelete(handle->priv, key, strlen(key) + 1);
}

bool NewDBCursor(DBHandle *handle, DBCursor **cursor)
{
    DBCursorPriv *priv = DBPrivOpenCursor(handle->priv);
    if (!priv)
    {
        return false;
    }

    *cursor = xcalloc(1, sizeof(DBCursor));
    (*cursor)->cursor = priv;
    return true;
}

bool NextDB(DBCursor *cursor, char **key, int *ksize,
            void **value, int *vsize)
{
    return DBPrivAdvanceCursor(cursor->cursor, (void **)key, ksize, value, vsize);
}

bool DBCursorDeleteEntry(DBCursor *cursor)
{
    return DBPrivDeleteCursorEntry(cursor->cursor);
}

bool DBCursorWriteEntry(DBCursor *cursor, const void *value, int value_size)
{
    return DBPrivWriteCursorEntry(cursor->cursor, value, value_size);
}

bool DeleteDBCursor(DBCursor *cursor)
{
    DBPrivCloseCursor(cursor->cursor);
    free(cursor);
    return true;
}

static int DBPathLock(const char *filename)
{
    char *filename_lock;
    if (xasprintf(&filename_lock, "%s.lock", filename) == -1)
    {
        ProgrammingError("Unable to construct lock database filename for file %s", filename);
    }

    int fd = open(filename_lock, O_CREAT | O_RDWR, 0666);

    if (fd == -1)
       {
       //Log(LOG_LEVEL_VERBOSE, "Unable to open database lock file '%s'. (flock: %s)", filename_lock, GetErrorStr());
       free(filename_lock);
       return -1;
       }
    
    if (ExclusiveLockFile(fd) == -1)
    {
        Log(LOG_LEVEL_VERBOSE, "Unable to lock database lock file '%s'. (fcntl(F_SETLK): %s)", filename_lock, GetErrorStr());
        free(filename_lock);
        close(fd);
        return -1;
    }

    free(filename_lock);

    return fd;
}

static void DBPathUnLock(int fd)
{
    if(ExclusiveUnlockFile(fd) != 0)
    {
        Log(LOG_LEVEL_VERBOSE, "Could not close db lock-file. (close: %s)", GetErrorStr());
    }
}

static void DBPathMoveBroken(const char *filename)
{
    char *filename_broken;
    if (xasprintf(&filename_broken, "%s.broken", filename) == -1)
    {
        ProgrammingError("Unable to construct broken database filename for file '%s'", filename);
    }

    if(rename(filename, filename_broken) != 0)
    {
        Log(LOG_LEVEL_VERBOSE, "Failed moving broken db out of the way");
    }

    free(filename_broken);
}

StringMap *LoadDatabaseToStringMap(dbid database_id)
{
    CF_DB *db_conn = NULL;
    CF_DBC *db_cursor = NULL;
    char *key = NULL;
    void *value = NULL;
    int key_size = 0;
    int value_size = 0;

    if (!OpenDB(&db_conn, database_id))
    {
        return NULL;
    }

    if (!NewDBCursor(db_conn, &db_cursor))
    {
        Log(LOG_LEVEL_VERBOSE, "Unable to scan db");
        CloseDB(db_conn);
        return NULL;
    }

    StringMap *db_map = StringMapNew();
    while (NextDB(db_cursor, &key, &key_size, &value, &value_size))
    {
        if (!key)
        {
            continue;
        }

        if (!value)
        {
            Log(LOG_LEVEL_VERBOSE, "Invalid entry (key='%s') in database.", key);
            continue;
        }

        void *val = xcalloc(1, value_size);
        val = memcpy(val, value, value_size);

        StringMapInsert(db_map, xstrdup(key), val);
    }

    DeleteDBCursor(db_cursor);
    CloseDB(db_conn);

    return db_map;
}

