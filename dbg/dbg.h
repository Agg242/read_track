/*

** File: dbg.h

*/

#ifndef dbg_h__
#define dbg_h__

#include <stdio.h>
#include <errno.h>
#include <string.h>

#define ASSIGNED(x) (x != NULL)

#ifndef DEBUG
#define debug(M, ...)
#define time_debug(M, ...)
#else

#define __PRIV_debug(fmt, ...) fprintf(stderr, "DEBUG %s:%d: "fmt"\n", __FILE__, __LINE__, __VA_ARGS__)
#define debug(...) __PRIV_debug(__VA_ARGS__, 0)

#define __PRIV_time_debug(fmt, ...) fprintf(stderr, "DEBUG %d: "fmt"\n", __FILE__, __LINE__, __VA_ARGS__)
#define time_debug(...) __PRIV_time_debug(__VA_ARGS__, 0)

#endif


#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define __PRIV_log(lvl, fmt, ...) fprintf(stderr, "[" lvl "] (%s:%d: errno: %s) " fmt "\n", __FILE__, __LINE__, clean_errno(), __VA_ARGS__)

#define log_err(...) __PRIV_log("ERROR", __VA_ARGS__, 0)

#define log_warn(...) __PRIV_log("WARN", __VA_ARGS__, 0)

#define log_info(...) __PRIV_log("INFO", __VA_ARGS__, 0)

#define check(A, ...) if(!(A)) { log_err(__VA_ARGS__); errno=0; goto on_error; }
#define sentinel(...)  { log_err(__VA_ARGS__); errno=0; goto on_error; }
#define check_mem(A) check((A), "Out of memory.")
#define check_debug(A, ...) if(!(A)) { debug(__VA_ARGS__); errno=0; goto on_error; }

#endif

