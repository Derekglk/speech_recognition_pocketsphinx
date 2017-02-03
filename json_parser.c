#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "whatever.h"

#ifndef TEST
//#define TEST
#endif

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
  result->det_location = LOCATION_INV;
  result->det_object = OBJECT_INV;
}

static void detect_object(char *command, result_t *result) {
  int i;

#if defined TEST
  printf("object size = %lu\n", sizeof(object));
  for (i = 0; i < (sizeof(object)/sizeof(object[0])); i++) {
      printf("element[%d] = %s\n", i, object[i].name);
  }
#endif
  for (i = 0; i < (sizeof(object)/sizeof(object[0])); i++) {
      if (strstr(command, object[i].name) != NULL) {
	  result->det_object = object[i].enum_value;
	  printf("object [%s] detected!\n", object[i].name);
	  break;
      }
  }
  if (result->det_object == OBJECT_INV) {
      printf("XXXXXXXXXXXXXXXX[NO OBJECT]XXXXXXXXXXXXXXXX\n");
  }
  return;
}

static void detect_location(char *command, result_t *result) {
  int i;

  for (i = 0; i < (sizeof(location)/sizeof(location[0])); i++) {
      if (strstr(command, location[i].name) != NULL) {
	  result->det_location = location[i].enum_value;
	  printf("location [%s] detected!\n", location[i].name);
	  break;
      }
  }
  if (result->det_location == LOCATION_INV) {
      printf("XXXXXXXXXXXXXXXX[NO LOCATION]XXXXXXXXXXXXXXXX\n");
  }
  return;
}

static void detect_action(char *command, result_t *result) {
  int i;

  for (i = 0; i < (sizeof(action)/sizeof(action[0])); i++) {
      if (strstr(command, action[i].name) != NULL) {
	  result->det_action = action[i].enum_value;
	  printf("action [%s] detected!\n", action[i].name);
	  break;
      }
  }
  if (result->det_action == ACTION_INV) {
      printf("XXXXXXXXXXXXXXXX[NO ACTION]XXXXXXXXXXXXXXXX\n");
  }
  return;
}

static int check_result(result_t *result) {
  if (result->det_object == OBJECT_INV) {
      printf("UNKONW object!\n");
      return -1;
  }
  if (result->det_action == ACTION_INV) {
      printf("UNKNOWN action!\n");
      return -1;
  }
  return 0;
}

static int proc_light(result_t *result) {
  if (result->det_location == LOCATION_INV) {
      printf("Please specify light location.\n");
      return -1;
  }
  return 0;
}

static int proc_window(result_t *result) {
  return 0;
}

static int proc_door(result_t *result) {
  return 0;
}

static int proc_radiator(result_t *result) {
  return 0;
}

static int processing(result_t *result) {
  int ret = 0;
  switch (result->det_object) {
    case LIGHT:
      ret = proc_light(result);
      break;
    case WINDOW:
      ret = proc_window(result);
      break;
    case DOOR:
      ret = proc_door(result);
      break;
    case RADIATOR:
      ret = proc_radiator(result);
      break;
    default:
      printf("OBJECT[%d] not recognized!\n", result->det_object);
  }
  return ret;
}


int command_proc(const char *command){

#if defined TEST
  char phrase_test[] = "TURN-ON THE LIVING-ROOM LIGHT";
  char action_test[] = "TURN-ON";
  char object_test[] = "window";
  char location_test[] = "LIVING-ROOM";


  if (strstr(phrase_test, action_test) != NULL) {
      printf("phrase includes action!\n");
  } else {
      printf("phrase NOT includes action!\n");
  }
  if (strstr(phrase_test, object_test) != NULL) {
      printf("phrase includes object!\n");
  } else {
      printf("phrase NOT includes object!\n");
  }
  if (strstr(phrase_test, location_test) != NULL) {
      printf("phrase includes location!\n");
  } else {
      printf("phrase NOT includes location!\n");
  }

  char switch_content[] = "computer";
  char phrase_addr[] = "computer";

  switch (switch_content) {
    case "computer":
      printf("computer case\n");
      break;
    case "screen":
      printf("screen case\n");
      break;
    case "window":
      printf("window case\n");
      break;
    default:
      printf("nobody's case\n");
      break;
  }

  printf("switch_content = 0x%08ld\n", (long)switch_content);
  printf("phrase_addr = 0x%08ld\n", (long)phrase_addr);
#endif
  int i;
  result_t result;

  printf("receive command: [%s]\n", command);
  init_result(&result);

  detect_object(command, &result);
  detect_location(command, &result);
  detect_action(command, &result);

  if (check_result(&result) != 0) {
      printf("check result failed!\n");
      return -1;
  }
  if (processing(&result) != 0) {
      printf("processing result failed!\n");
      return -1;
  }
  return 0;
}
