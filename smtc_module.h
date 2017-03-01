#include <stdio.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"


#define LOG_RED(X) printf("%s%s%s",RED,X,RESET)
#define LOG_GREEN(X) printf("%s%s%s",GREEN,X,RESET)
#define LOG_YELLOW(X) printf("%s%s%s",YELLOW,X,RESET)
#define LOG_BLUE(X) printf("%s%s%s",BLUE,X,RESET)

typedef enum {
  LIGHT,
  WINDOW,
  DOOR,
  RADIATOR,
  OBJECT_NUM,
  OBJECT_INV = 255
} object_enum;

typedef enum {
  OPEN,
  CLOSE,
  TURN_ON,
  TURN_OFF,
  TURN_UP,
  TURN_DOWN,
  SWITCH_ON,
  SWITCH_OFF,
  LOCK,
  UNLOCK,
  ACTION_NUM,
  ACTION_INV = 255,
} action_enum;

typedef enum {
  LIVING_ROOM,
  KITCHEN,
  BATHROOM,
  LOCATION_NUM,
  LOCATION_INV = 255,
} location_enum;

typedef struct result_s {
  object_enum det_object;
  location_enum det_location;
  action_enum det_action;
} result_t;

int command_proc(int pipe_write, const char *command);

int dummy_commander(int pipe_read, char *addr, char *port);
