#ifndef LOG_H
#define LOG_H

#include "types.h"

/* Log levels */
#define LOG_EMERG   0
#define LOG_ALERT   1
#define LOG_CRIT    2
#define LOG_ERR     3
#define LOG_WARNING 4
#define LOG_NOTICE  5
#define LOG_INFO    6
#define LOG_DEBUG   7

/* Log facility */
#define LOG_KERN    0
#define LOG_USER    1
#define LOG_DAEMON  3

/* Log buffer */
#define LOG_BUFFER_SIZE (64 * 1024)

typedef struct log_entry {
    u64 timestamp;
    u8 level;
    u8 facility;
    char message[256];
} log_entry_t;

/* Initialize logging system */
void log_init(void);

/* Log message */
void klog(u8 level, u8 facility, const char* format, ...);

/* Log macros */
#define log_emerg(fmt, ...)   klog(LOG_EMERG, LOG_KERN, fmt, ##__VA_ARGS__)
#define log_alert(fmt, ...)   klog(LOG_ALERT, LOG_KERN, fmt, ##__VA_ARGS__)
#define log_crit(fmt, ...)    klog(LOG_CRIT, LOG_KERN, fmt, ##__VA_ARGS__)
#define log_err(fmt, ...)     klog(LOG_ERR, LOG_KERN, fmt, ##__VA_ARGS__)
#define log_warning(fmt, ...) klog(LOG_WARNING, LOG_KERN, fmt, ##__VA_ARGS__)
#define log_notice(fmt, ...)  klog(LOG_NOTICE, LOG_KERN, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)    klog(LOG_INFO, LOG_KERN, fmt, ##__VA_ARGS__)
#define log_debug(fmt, ...)   klog(LOG_DEBUG, LOG_KERN, fmt, ##__VA_ARGS__)

/* Read log */
int log_read(log_entry_t* entries, u32 count);

/* Clear log */
void log_clear(void);

#endif /* LOG_H */
