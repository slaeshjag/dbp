#ifndef __DBP_UTIL_H__
#define	__DBP_UTIL_H__

#if __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
char* dbp_string(const char *fmt, ...);

#endif
