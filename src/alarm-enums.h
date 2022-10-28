typedef enum {
    ALARM_TYPE_INVALID = 0,
    ALARM_TYPE_CLOCK, /* Alarm at specific time */
    ALARM_TYPE_TIMER, /* Alarm in X mins */
} AlarmType;

typedef enum {
    ALARM_REPEAT_NONE = 0,
    ALARM_REPEAT_SUN = 1 << 0,
    ALARM_REPEAT_MON = 1 << 1,
    ALARM_REPEAT_TUE = 1 << 2,
    ALARM_REPEAT_WED = 1 << 3,
    ALARM_REPEAT_THU = 1 << 4,
    ALARM_REPEAT_FRI = 1 << 5,
    ALARM_REPEAT_SAT = 1 << 6,
} AlarmRepeat;

typedef enum {
    ALARM_NOTIFY_INVALID = 0,
    ALARM_NOTIFY_SOUND,   /* Notification by sound */
    ALARM_NOTIFY_COMMAND, /* Notification by command */
} AlarmNotifyType;
