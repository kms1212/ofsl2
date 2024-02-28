#ifndef OFSL_CONF_H__
#define OFSL_CONF_H__

/* compiler attributes */
#ifndef OFSL_INLINE
#define OFSL_INLINE    __attribute__((always_inline))
#endif

#ifndef OFSL_DEPRECATED
#define OFSL_DEPRECATED(msg) __attribute__((deprecated(msg)))
#endif

#ifndef OFSL_FORMAT
#define OFSL_FORMAT(...) __attribute__((format(__VA_ARGS__)))
#endif

#endif
