#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "smtc_module.h"

typedef struct str2enum_s {
  const char *name;
  int enum_value;
} str2enum_t;

static str2enum_t object[] = {
    {"LIGHT", LIGHT},
    {"WINDOW", WINDOW},
    {"DOOR", DOOR},
    {"RADIATOR", RADIATOR}
};

static str2enum_t location[] = {
    {"LIVING-ROOM", LIVING_ROOM},
    {"KITCHEN", KITCHEN},
    {"BATHROOM", BATHROOM}
};

static str2enum_t action[] = {
    {"OPEN", OPEN},
    {"CLOSE", CLOSE},
    {"TURN-ON", TURN_ON},
    {"TURN-OFF", TURN_OFF},
    {"TURN-UP", TURN_UP},
    {"TURN-DOWN", TURN_DOWN},
    {"SWITCH-ON", SWITCH_ON},
    {"SWITCH-OFF", SWITCH_OFF},
    {"LOCK", LOCK},
    {"UNLOCK", UNLOCK}
};

static void init_result(result_t *result) {
  result->det_action = ACTION_INV;
  result->det_location = ANYWHERE;
  result->det_object = OBJECT_INV;
}

static void detect_object(const char *command, result_t *result) {
  int i;

  for (i = 0; i < (sizeof(object)/sizeof(object[0])); i++) {
      if (strstr(command, object[i].name) != NULL) {
	  result->det_object = object[i].enum_value;
	  LOG_YELLOW("===>object ");
	  LOG_BLUE("[");
	  LOG_BLUE(object[i].name);
	  LOG_BLUE("]");
	  LOG_YELLOW("detected!\n");
	  break;
      }
  }

  return;
}

static void detect_location(const char *command, result_t *result) {
  int i;

  for (i = 0; i < (sizeof(location)/sizeof(location[0])); i++) {
      if (strstr(command, location[i].name) != NULL) {
	  result->det_location = location[i].enum_value;
	  LOG_YELLOW("===>location ");
	  LOG_BLUE("[");
	  LOG_BLUE(location[i].name);
	  LOG_BLUE("]");
	  LOG_YELLOW("detected!\n");
	  break;
      }
  }

  return;
}

static void detect_action(const char *command, result_t *result) {
  int i;

  for (i = 0; i < (sizeof(action)/sizeof(action[0])); i++) {
      if (strstr(command, action[i].name) != NULL) {
	  result->det_action = action[i].enum_value;
	  LOG_YELLOW("===>action ");
	  LOG_BLUE("[");
	  LOG_BLUE(action[i].name);
	  LOG_BLUE("]");
	  LOG_YELLOW("detected!\n");
	  break;
      }
  }

  return;
}

static int check_result(result_t *result) {
  if (result->det_object == OBJECT_INV) {
      LOG_RED("===>[error]UNKONW object!\n");
      return -1;
  }
  if (result->det_action == ACTION_INV) {
      LOG_RED("===>[error]UNKNOWN action!\n");
      return -1;
  }

  return 0;
}

static int processing(int pipe_write, result_t *result) {
  int ret = 0;

  if (write(pipe_write, result, sizeof(result_t)) < 0) {
      perror("send semantic result");
      ret = -1;
  }
  return ret;
}


int command_proc(int pipe_write, const char *command){

  result_t result;

  LOG_YELLOW("*******PROCESSING MODULE*******\n");
  LOG_YELLOW("===>receive command: ");
  LOG_BLUE("[");
  LOG_BLUE(command);
  LOG_BLUE("]\n");

  init_result(&result);

  detect_object(command, &result);
  detect_location(command, &result);
  detect_action(command, &result);

  if (check_result(&result) != 0) {
      LOG_RED("[error]check result failed!\n");
      return -1;
  }
  if (processing(pipe_write, &result) != 0) {
      LOG_RED("[error]processing result failed!\n");
      return -1;
  }
  return 0;
}
