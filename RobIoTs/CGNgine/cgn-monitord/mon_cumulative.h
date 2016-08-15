/*
 This file is (C) Cfengine AS. See COSL LICENSE for details.
*/

#ifndef CFENGINE_MON_CUMULATIVE_H
#define CFENGINE_MON_CUMULATIVE_H

unsigned GetInstantUint32Value(const char *name, const char *subname, unsigned value, time_t timestamp);
unsigned long long GetInstantUint64Value(const char *name, const char *subname, unsigned long long value,
                                         time_t timestamp);

#endif
