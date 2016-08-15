/*

 This file is (C) Cfengine AS. See COSL LICENSE for details.

*/

#ifndef CFENGINE_HISTORY_H
#define CFENGINE_HISTORY_H
#include <cf3.defs.h>

PromiseResult VerifyMeasurement(EvalContext *ctx,  double *this, Attributes a, const Promise * pp);
void HistoryUpdate(EvalContext *ctx, Averages newvals);
void MakeTimekey(time_t time, char *result);
#endif
