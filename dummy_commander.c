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


/* Send a request to a set of lamps*/
/* Return true if success */
static bool bulk_request(const xAAL_businfo_t *bus, const xAAL_devinfo_t *cli,
			 lamps_t *lamps, const char *action) {
  lamp_t *np;
  char **targets;
  int i = 0, max = 10;
  bool r;

  /* Build a list of targets to query */
  targets = (char **)malloc(max*sizeof(char *));
  LIST_FOREACH(np, lamps, entries)
    if ( np->timeout && (time(NULL) > np->timeout) ) {
      // Too old, clean it
      LIST_REMOVE(np, entries);
      free(np->addr);
      free(np->type);
      free(np);
    } else if (np->selected) {
      targets[i++] = np->addr;
      if (i == max) {
	max+=10;
	targets = (char **)realloc(targets, max*sizeof(char *));
      }
    }
  targets[i] = NULL;

  r = xAAL_write_busv(bus, cli, "request", action, NULL, targets);
  free(targets);
  return r;
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




/* Command Line Interface */
static int command_hdlr(int pipe_read, const xAAL_businfo_t *bus,
			 const xAAL_devinfo_t *cli, lamps_t *lamps) {

//  lamp_t *np;
  result_t result;
//  int i;

  if (read(pipe_read, &result, sizeof(result_t)) < 0) {
      perror("read semantic result");
      return -1;
  }

  printf("object = [%d], location = [%d], action = [%d]\n",
	 result.det_object, result.det_location, result.det_action);
  return 0;
#if 0
  if ( scanf("%d", &menu) == 1 ) {
    switch (menu) {
    case 1:
      i = 0;
      printf("Detected lamps:\n");
      LIST_FOREACH(np, lamps, entries) {
	if ( np->timeout && (time(NULL) > np->timeout) ) {
	  LIST_REMOVE(np, entries);
	  free(np->addr);
	  free(np->type);
	  free(np);
	} else
	  printf("%2d: %c %s %s %s", i++, np->selected?'*':' ',
		 np->addr, np->type, ctime(&np->timeout));
      }
      if (i) {
	printf("Toggle which one? ");
	fflush(stdout);
	if ( scanf("%d", &choice) == 1 ) {
	  i = 0;
	  bool yes = false;
	  LIST_FOREACH(np, lamps, entries)
	    if ( choice == i++ ) {
	      yes = true;
	      np->selected = !np->selected;
	      break;
	    }
	  if (!yes)
	    printf("Sorry, can't find it.\n");
	} else {
	  scanf("%*s");
	  printf("Sorry.\n");
	}
      }
      break;
    case 2:
      if (!bulk_request(bus, cli, lamps, "on"))
	fprintf(xAAL_error_log, "Could not send 'on' request\n");
      break;
    case 3:
      if (!bulk_request(bus, cli, lamps, "off"))
	fprintf(xAAL_error_log, "Could not send 'off' request\n");
      break;
    case 4:
      exit(EXIT_SUCCESS);
    default:
      printf("Sorry, %d is not on the menu.\n", menu);
    }
  } else {
    scanf("%*s");
    printf("Sorry.\n");
  }
#endif
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
  cli.productId  = "Lamp Commander";
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

  if ( !xAAL_notify_alive(&bus, &cli) )
    fprintf(xAAL_error_log, "Could not send initial alive notification.\n");

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
      command_hdlr(pipe_read, &bus, &cli, &lamps);

    } else if (FD_ISSET(bus.sfd, &rfds_)) {
      /* Recive a message */
      manage_msg(&bus, &cli, &lamps);

    }
  }
}
