#ifndef EXPORT_H__
#define EXPORT_H__

#define OFSL_CTOR               __attribute__((constructor))
#define OFSL_DTOR               __attribute__((destructor))
#define OFSL_EXPORT             __attribute__((visibility("default")))
#define OFSL_INTERNAL           __attribute__((visibility("internal")))
#define OFSL_HIDDEN             __attribute__((visibility("hidden")))
#define OFSL_WEAK               __attribute__((weak))
#define OFSL_PACKED             __attribute__((packed))

#endif
