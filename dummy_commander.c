/* Lamp Commander - xAAL dummy hmi in command line for lamps
 * (c) 2014 Christophe Lohr <christophe.lohr@telecom-bretagne.eu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/queue.h>
#include <json-c/json.h>
#include <uuid/uuid.h>

#include <xaal.h>

#include "smtc_module.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_CONFIG_SIZE (1024)
#define CONFIG_FILE "device_config.json"

typedef struct enum2xaal_s {
  int enum_value;
  const char *str;
} enum2xaal_t;

static enum2xaal_t object_xaal[] = {
    {LIGHT, "lamp"},
    {WINDOW, "shutter"},
    {DOOR, "door"},
    {RADIATOR, "thermometer"}
};

static enum2xaal_t location_xaal[] = {
    {LIVING_ROOM, "livingroom"},
    {KITCHEN, "kitchen"},
    {BATHROOM, "bathroom"}
};

static enum2xaal_t action_xaal[] = {
    {OPEN, "open"},
    {CLOSE, "close"},
    {TURN_ON, "on"},
    {TURN_OFF, "off"},
    {TURN_UP, "up"},
    {TURN_DOWN, "down"},
    {SWITCH_ON, "on"},
    {SWITCH_OFF, "off"},
    {LOCK, "lock"},
    {UNLOCK, "unlock"}
};

/* Some global variables nedded for the sig handler */
xAAL_businfo_t bus;
xAAL_devinfo_t cli;

/* List of detected lamps */
typedef LIST_HEAD(listhead, entry) lamps_t;
typedef struct entry {
  char *addr;
  char *type;
  bool selected;
  time_t timeout;
  LIST_ENTRY(entry) entries;
} lamp_t;

#define ALIVE_PERIOD	60

/* SIGALRM handler that sends alive messages */
static void alive_sender(int sig) {
  if (!xAAL_notify_alive(&bus, &cli) )
    fprintf(xAAL_error_log, "Could not send spontaneous alive notification.\n");
  alarm(ALIVE_PERIOD);
}


/* Send isAlive request */
/* Return true if success */
static bool request_isAlive(const xAAL_businfo_t *bus, const xAAL_devinfo_t *cli) {
  struct json_object *jbody, *jdevTypes;

  jdevTypes = json_object_new_array();
  json_object_array_add(jdevTypes, json_object_new_string("lamp.any"));

  jbody = json_object_new_object();
  json_object_object_add(jbody, "devTypes", jdevTypes);

  return xAAL_write_busl(bus, cli, "request", "isAlive", jbody, NULL);
}

static int search_and_send_cmd(const xAAL_businfo_t *bus, const xAAL_devinfo_t *cli,
			       const char *object, const char *location,
			       const char *action) {
  int fd;
  int i, len;
  int ret = 0;
  int idx = 0, max = 10;
  char config[MAX_CONFIG_SIZE];
  char *dev_name;
  char **targets;
  const char *uuid_conf, *type_conf, *location_conf;

  struct stat fd_stat;
  ssize_t nread;
  json_type type;
  struct json_tokener *tok;
  enum json_tokener_error jerr;
  json_object *jobj, *record, *juuid, *jtype, *jlocation;


  targets = (char **)malloc(max*sizeof(char *));

  fd = open(CONFIG_FILE, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd < 0) {
      perror("open config file");
      return -1;
  }

  if (fstat(fd, &fd_stat) < 0) {
      perror("fstat config file");
      close(fd);
      return -1;
  }
  if (fd_stat.st_size > MAX_CONFIG_SIZE) {
      printf("config file size too large\n");
      close(fd);
      return -1;
  }

  nread = read(fd, config, MAX_CONFIG_SIZE);
  if (nread < 0) {
      perror("read config file");
      close(fd);
      return -1;
  }
  printf("file read success\n");
  tok = json_tokener_new_ex(JSON_TOKENER_DEFAULT_DEPTH);
  jobj = json_tokener_parse_ex(tok, config, nread);
  jerr = json_tokener_get_error(tok);
  json_tokener_free(tok);
  if (jerr != json_tokener_success) {
    printf("JSON error: %s\n", json_tokener_error_desc(jerr));
    return -1;
  }
  printf("JSON parse expression success\n");
  type = json_object_get_type(jobj);
  if (type != json_type_array) {
      printf("config file syntax error. should be array\n");
      return -1;
  }

  len = json_object_array_length(jobj);
    printf("Array Length: %d\n",len);

  for (i = 0; i < len; i++) {
      record = json_object_array_get_idx(jobj, i);
      if (json_object_get_type(record) == json_type_object) {
	if ( !json_object_object_get_ex(record, "uuid", &juuid)
	     || json_object_get_type(juuid) != json_type_string ) {
	  printf("Invalid 'uuid' in record\n");
	  ret = -1;
	}
	if ( !json_object_object_get_ex(record, "dev_type", &jtype)
	     || json_object_get_type(jtype) != json_type_string ) {
	  printf("Invalid 'dev_type' in record\n");
	  ret = -1;
	}
	if ( !json_object_object_get_ex(record, "location", &jlocation)
	     || json_object_get_type(jlocation) != json_type_string ) {
	  printf("Invalid 'location' in record\n");
	  ret = -1;
	}
	uuid_conf = json_object_get_string(juuid);
	type_conf = json_object_get_string(jtype);
	location_conf = json_object_get_string(jlocation);

	printf("uuid = %s\n", uuid_conf);
	printf("type = %s\n", type_conf);
	printf("location = %s\n", location_conf);
	printf("desired object = %s\n", object);
	printf("desired location = %s\n", location);
	printf("desired action = %s\n", action);

      } else {
	printf("config list element is NOT object\n");
	ret = -1;
      }
      if (ret != -1) {
	  sprintf(dev_name, "%s%s", object, ".");
	  if ((strncmp(type_conf, dev_name, strlen(dev_name)) == 0) &&
	      (strncmp(location_conf, location, strlen(location)) == 0)) {
	      targets[idx++] = (char *)uuid_conf;
	  }
      }
  }
  targets[idx] = NULL;
  if (ret != -1)
  	ret = xAAL_write_busv(bus, cli, "request", action, NULL, targets);
  free(targets);
  return ret;
}

static int command_hdlr(int pipe_read, const xAAL_businfo_t *bus,
			 const xAAL_devinfo_t *cli) {
  result_t result;
  int i;
  const char *object, *location, *action;

  if (read(pipe_read, &result, sizeof(result_t)) < 0) {
      perror("read semantic result");
      return -1;
  }

  printf("object = [%d], location = [%d], action = [%d]\n",
	 result.det_object, result.det_location, result.det_action);
  for (i = 0; i < sizeof(object_xaal)/sizeof(object_xaal[0]); i++) {
      if (object_xaal[i].enum_value == result.det_object) {
	  object = object_xaal[i].str;
	  break;
      }
  }
  for (i = 0; i < sizeof(location_xaal)/sizeof(location_xaal[0]); i++) {
      if (location_xaal[i].enum_value == result.det_location) {
	  location = location_xaal[i].str;
	  break;
      }
  }
  for (i = 0; i < sizeof(action_xaal)/sizeof(action_xaal[0]); i++) {
      if (action_xaal[i].enum_value == result.det_action) {
	  action = action_xaal[i].str;
	  break;
      }
  }
  printf("object = %s, location = %s, action = %s\n", object, location, action);

  search_and_send_cmd(bus, cli, object, location, action);

  return 0;
}

/* manage received message */
static void manage_msg(const xAAL_businfo_t *bus, const xAAL_devinfo_t *cli, lamps_t *lamps) {
  struct json_object *jmsg, *jtargets;
  const char *version, *source, *msgType, *devType, *action,
	     *cipher, *signature;
  time_t timestamp;
  lamp_t *np;

  if (!xAAL_read_bus(bus, &jmsg, &version, &source, &jtargets, &msgType, &devType,
		&action, &cipher, &signature, &timestamp))
    return;

  if (    (strcmp(msgType, "request") == 0)
       && xAAL_targets_match(jtargets, cli->addr) ) {

    if ( (strcmp(action, "isAlive") == 0)
	&& xAAL_isAliveDevType_match(jmsg, cli->devType) ) {
      if ( !xAAL_notify_alive(bus, cli) )
	fprintf(xAAL_error_log, "Could not reply to isAlive\n");

    } else if ( strcmp(action, "getDescription") == 0 ) {
      if ( !xAAL_reply_getDescription(bus, cli, source) )
	fprintf(xAAL_error_log, "Could not reply to getDescription\n");

    } else if ( strcmp(action, "getAttributes") == 0 ) {
      /* I have no attributes */

    } else if ( strcmp(action, "getBusConfig") == 0 ) {
      if ( !xAAL_reply_getBusConfig(bus, cli, source) )
	fprintf(xAAL_error_log, "Could not reply to getBusConfig\n");

    } else if ( strcmp(action, "setBusConfig") == 0 ) {
      if ( !xAAL_reply_setBusConfig(bus, cli, source) )
	fprintf(xAAL_error_log, "Could not reply to setBusConfig\n");

    } else if ( strcmp(action, "getCiphers") == 0 ) {
      if ( !xAAL_reply_getCiphers(bus, cli, source) )
	fprintf(xAAL_error_log, "Could not reply to getCiphers\n");

    } else if ( strcmp(action, "setCiphers") == 0 ) {
      if ( !xAAL_reply_setCiphers(bus, cli, source) )
	fprintf(xAAL_error_log, "Could not reply to setCiphers\n");
    }

  } else if (strncmp(devType, "lamp.", strlen("lamp.")) == 0) {
    /* A lamp is talking */
    bool exists = false;
    LIST_FOREACH(np, lamps, entries)
      if ( strcmp(np->addr, source) == 0 ) {
	if ( (strcmp(msgType, "notify") == 0) && (strcmp(action, "alive") == 0) )
	  np->timeout = xAAL_read_aliveTimeout(jmsg);
	else
	  np->timeout = 0;
	exists = true;
	break;
      }
    if (!exists) {
      np = malloc(sizeof(lamp_t));
      np->addr = strdup(source);
      np->type = strdup(devType);
      if ( (strcmp(msgType, "notify") == 0) && (strcmp(action, "alive") == 0) )
	np->timeout = xAAL_read_aliveTimeout(jmsg);
      else
	np->timeout = 0;
      np->selected = false;
      LIST_INSERT_HEAD(lamps, np, entries);
    }

  }
  json_object_put(jmsg);
}



/* main */
int dummy_commander(int pipe_read, char *addr, char *port) {

  uuid_t uuid;
  int hops = -1;

  struct sigaction act_alarm;
  fd_set rfds, rfds_;
  lamps_t lamps;

  uuid_clear(uuid);

  /* Join the xAAL bus */
  xAAL_error_log = stderr;
  if (!xAAL_join_bus(addr, port, hops, 1, &bus))
    exit(EXIT_FAILURE);

  /* Generate device address if needed */
  if ( uuid_is_null(uuid) ) {
    uuid_generate(uuid);
    uuid_unparse(uuid, cli.addr);
    printf("Device: %s\n", cli.addr);
  }

  /* Setup 'cli' device info */
  cli.devType    = "hmi.basic";
  cli.alivemax   = 2 * ALIVE_PERIOD;
  cli.vendorId   = "Team IHSEV";
  cli.productId  = "Dummy Commandor";
  cli.hwId	 = NULL;
  cli.version    = "0.3";
  cli.parent     = "";
  cli.childrens	 = (char *[]){ NULL };
  cli.url	 = "http://recherche.telecom-bretagne.eu/xaal/documentation/";
  cli.info	 = NULL;
  cli.unsupportedAttributes = NULL;
  cli.unsupportedMethods = (char *[]){ "getAttributes", NULL };
  cli.unsupportedNotifications = (char *[]){ "attributesChange", NULL };

  /* Setup 'lamps' list */
  LIST_INIT(&lamps);

  /* Manage periodic alive notifications */
  act_alarm.sa_handler = alive_sender;
  act_alarm.sa_flags = ~SA_RESETHAND | SA_RESTART;
  sigemptyset(&act_alarm.sa_mask);
  sigaction(SIGALRM, &act_alarm, NULL);
  alarm(ALIVE_PERIOD);

  /* notify alive is to tell the others that i'm alive */
  if ( !xAAL_notify_alive(&bus, &cli) )
    fprintf(xAAL_error_log, "Could not send initial alive notification.\n");

  /* request isAlive is to ask who's alive on the bus */
  if ( !request_isAlive(&bus, &cli) )
    fprintf(xAAL_error_log, "Could not send isAlive request.\n");


  FD_ZERO(&rfds);
  FD_SET(pipe_read, &rfds);
  FD_SET(bus.sfd, &rfds);

  /* Main loop */
  for (;;) {
    rfds_ = rfds;
    if ( (select(bus.sfd+1, &rfds_, NULL, NULL, NULL) == -1) && (errno != EINTR) )
      fprintf(xAAL_error_log, "select(): %s\n", strerror(errno));

    if (FD_ISSET(pipe_read, &rfds_)) {
      /* User wake up */
      command_hdlr(pipe_read, &bus, &cli);

    } else if (FD_ISSET(bus.sfd, &rfds_)) {
      /* Recive a message */
      manage_msg(&bus, &cli, &lamps);

    }
  }
}
