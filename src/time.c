#include <ofsl/time.h>

#include <time.h>

#include "config.h"

extern long timezone;

static int verify_timespec(const struct timespec* ts)
{
    return !ts || ts->tv_sec < 0 || ts->tv_nsec < 0 || ts->tv_nsec > 999999999;
}

OFSL_EXPORT
int ofsl_time_getlocal(OFSL_Time* fstm)
{
#if __STDC_VERSION__ < 201112L
    time_t now = time(NULL);
    struct tm now_tm;
    if (!localtime_r(&tm, &now_tm)) {
        return 1;
    }
    now = timegm(&now_tm);

    fstm->second = now;
    fstm->nsec = 0;
    
#else
    struct timespec ts;
    if (timespec_get(&ts, timezone) || verify_timespec(&ts)) {
        return 1;
    }

    fstm->second = ts.tv_sec;
    fstm->nsec = ts.tv_nsec;

#endif

    return 0;
}

OFSL_EXPORT
int ofsl_time_getutc(OFSL_Time* fstm)
{
#if __STDC_VERSION__ < 201112L
    time_t now = time(NULL);
    struct tm now_tm;
    if (!gmtime_r(&tm, &now_tm)) {
        return 1;
    }
    now = timegm(&now_tm);

    fstm->second = now;
    fstm->nsec = 0;
    
#else
    struct timespec ts;
    if (timespec_get(&ts, TIME_UTC) || verify_timespec(&ts)) {
        return 1;
    }

    fstm->second = ts.tv_sec;
    fstm->nsec = ts.tv_nsec;

#endif

    return 0;
}

OFSL_EXPORT
void ofsl_time_calcdiff(OFSL_Time* fstm, const OFSL_TimeDelta* tdelta)
{
    fstm->second += tdelta->dsecond;

    /* borrow */
    if (-tdelta->dnsec > fstm->nsec) {
        fstm->second -= 1;
        fstm->nsec += 1000000000;
    }

    fstm->nsec += tdelta->dnsec;
}

OFSL_EXPORT
void ofsl_time_getdiff(OFSL_TimeDelta* tdelta, const OFSL_Time* ref, const OFSL_Time* tm)
{
    tdelta->dsecond = tm->second - ref->second;
    tdelta->dnsec = tm->nsec - ref->nsec;
}

OFSL_EXPORT
int ofsl_time_getstdctm(struct tm* tm, const OFSL_Time* fstm)
{
    return !gmtime_r(&fstm->second, tm);
}

OFSL_EXPORT
void ofsl_time_fromstdctm(OFSL_Time* fstm, struct tm* tm)
{
    fstm->second = timegm(tm);
    fstm->nsec = 0;
}
