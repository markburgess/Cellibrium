/*

 This file is (C) Cfengine AS. See COSL LICENSE for details.

*/

#ifndef CFENGINE_MONITORING_H
#define CFENGINE_MONITORING_H
#include <cf3.defs.h>

void GetObservable(int i, char *name, char *desc);
void SetMeasurementPromises(Item **classlist);
void LoadSlowlyVaryingObservations(EvalContext *ctx);
int MonRegisterSlot(const char *name, const char *description, const char *units, double expected_minimum, double expected_maximum, bool consolidable);
bool MonHasSlot(int idx);
const char *MonGetSlotName(int idx);
const char *MonGetSlotDescription(int idx);
const char *MonGetSlotUnits(int idx);
double MonGetSlotExpectedMinimum(int idx);
double MonGetSlotExpectedMaximum(int idx);
bool MonIsSlotConsolidable(int idx);
void MonNamedEvent(const char *eventname, double value);
time_t WeekBegin(time_t time);
time_t SubtractWeeks(time_t time, int weeks);
time_t NextShift(time_t time);
bool GetRecordForTime(CF_DB *db, time_t time, Averages *result);
const int MonGetSlot(const char *name);
void Mon_DumpSlots(void);




#endif
