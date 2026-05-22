#ifndef AI_TYPES_H
#define AI_TYPES_H

#include "types.h"

/* Process workload class (heuristic) */
#define AI_CLASS_UNKNOWN  0
#define AI_CLASS_IDLE     1
#define AI_CLASS_CPU      2
#define AI_CLASS_IO       3
#define AI_CLASS_NET      4
#define AI_CLASS_MIXED    5

/* Scheduler policy presets */
#define AI_POLICY_BALANCED   0
#define AI_POLICY_LATENCY    1
#define AI_POLICY_THROUGHPUT 2
#define AI_POLICY_POWERSAVE  3
#define AI_POLICY_MAX        4

/* Logged decision types */
#define AI_ACT_NONE           0
#define AI_ACT_SLICE_REDUCE   1
#define AI_ACT_SLICE_INCREASE 2
#define AI_ACT_PRIO_BOOST     3
#define AI_ACT_PRIO_DEMOTE    4
#define AI_ACT_MEM_RECLAIM    5
#define AI_ACT_NET_BOOST      6
#define AI_ACT_ANOMALY        7
#define AI_ACT_POLICY_AUTO    8
#define AI_ACT_LEARN_REWARD   9
#define AI_ACT_VRUNTIME_TUNED 10
#define AI_ACT_ALERT          11
#define AI_ACT_GOAL_APPLY     12

/* User optimization goals */
#define AI_GOAL_NONE        0
#define AI_GOAL_LATENCY     1
#define AI_GOAL_THROUGHPUT  2
#define AI_GOAL_POWER       3
#define AI_GOAL_STABILITY   4

#define AI_USER_MAGIC 0x41494D31U

#define AI_LOG_SIZE 48
#define AI_HISTORY_LEN 24
#define AI_LEARN_ACTIONS 8

#define AI_AUTO_POLICY_OFF  0
#define AI_AUTO_POLICY_ON   1

typedef struct {
    u64 tick;
    u8 action;
    u8 policy;
    u32 pid;
    u32 value;
} ai_log_entry_t;

typedef struct {
    u64 cpu_ema;
    u64 mem_ema;
    u64 io_ema;
    u64 net_ema;
    u64 cpu_predict;
    u64 mem_predict;
    i64 cpu_trend;
    i64 mem_trend;
} ai_predict_state_t;

#endif /* AI_TYPES_H */
