#ifndef COMMUN_H
#define COMMUN_H

#define ONE_S_IN_NS                 1000000000UL
#define ONE_MS_IN_NS                (ONE_S_IN_NS/ 1000UL)
//------------------------------------------------
#define CANARY_PRIO                 40
#define CANARY_PERIOD_NS            10000000UL    // 10ms
//------------------------------------------------
#define WATCHDOG_PRIO               90
#define WATCHDOG_PERIOD_NS          (CANARY_PERIOD_NS * 10)  // = 100ms
#define WATCHDOG_MISSED_THRESHOLD   10 // Number of missed canary increments before termination
//------------------------------------------------
#define VIDEO_PRIO                  50
#define VIDEO_FRAMERATE             15
#define VIDEO_PERIOD_NS             (ONE_S_IN_NS / VIDEO_FRAMERATE)
//------------------------------------------------
#define LOAD_PRIO 70
#define LOAD_PERIOD_NS              (ONE_MS_IN_NS * 100)
//------------------------------------------------
typedef enum { VIDEO_MODE_NORMAL, VIDEO_MODE_DEGRADED_1, VIDEO_MODE_DEGRADED_2 } video_mode_t;
#define VIDEO_MODE_NORMAL_TRESHOLD 1
#define VIDEO_MODE_DEGRADED_1_TRESHOLD 3
#define VIDEO_MODE_DEGRADED_2_TRESHOLD 9


#endif