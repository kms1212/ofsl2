#ifndef OFSL_TIME_H__
#define OFSL_TIME_H__

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    time_t second;
    uint32_t nsec;
} OFSL_Time;

/**
 * @brief Time Delta Struct
 */
typedef struct {
    time_t dsecond;
    int32_t dnsec;
} OFSL_TimeDelta;

int ofsl_time_getlocal(OFSL_Time* tm);
int ofsl_time_getutc(OFSL_Time* tm);

void ofsl_time_calcdiff(OFSL_Time* fstm, const OFSL_TimeDelta* tdelta);
void ofsl_time_getdiff(OFSL_TimeDelta* tdelta, const OFSL_Time* ref, const OFSL_Time* tm);

int ofsl_time_getstdctm(struct tm* tm, const OFSL_Time* fstm);
void ofsl_time_fromstdctm(OFSL_Time* fstm, struct tm* tm);

#ifdef __cplusplus
};
#endif

#endif
