/* xAAL dummy basic lamp
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
#include <signal.h>

#include <json-c/json.h>
#include <uuid/uuid.h>

#include <xaal.h>




#define ALIVE_PERIOD    5

/* setup lamp device info */
xAAL_devinfo_t lamp = { .devType	= "lamp.basic",
			.alivemax	= 2 * ALIVE_PERIOD,
			.vendorId	= "Team IHSEV",
			.productId	= "Dummy basic lamp",
			.hwId		= NULL,
			.version	= "0.3",
			.parent		= "",
			.childrens	= (char *[]){ NULL },
			.url		= "http://recherche.telecom-bretagne.eu/xaal/documentation/",
			.info		= NULL,
			.unsupportedAttributes = NULL,
			.unsupportedMethods = (char *[]){ "getAttributes", NULL },
			.unsupportedNotifications = (char *[]){ "attributesChange", NULL }
		      };

xAAL_businfo_t bus;


void alive_sender(int sig) {
  if ( !xAAL_notify_alive(&bus, &lamp) )
    fprintf(xAAL_error_log, "Could not send spontaneous alive notification.\n");
  alarm(ALIVE_PERIOD);
}


/* main */
int main(int argc, char **argv) {
  int opt;
  char *addr=NULL, *port=NULL, *dev_name = NULL;
  uuid_t uuid;
  int hops = -1;
  bool arg_error = false;
  bool attribute_light = false;
  struct sigaction act_alarm;
  struct json_object *jmsg, *jtargets;
  const char *version, *source, *msgType, *devType, *action,
	     *cipher, *signature;
  time_t timestamp;

  xAAL_error_log = stderr;

  uuid_clear(uuid);

  /* Parse cmdline arguments */
  while ((opt = getopt(argc, argv, "a:p:h:u:n:")) != -1) {
    switch (opt) {
      case 'a':
	addr = optarg;
	break;
      case 'p':
	port = optarg;
	break;
      case 'h':
	hops = atoi(optarg);
	break;
      case 'u':
	if ( uuid_parse(optarg, uuid) == -1 ) {
	  fprintf(stderr, "Warning: invalid uuid '%s'\n", optarg);
	  uuid_clear(uuid);
	} else
	  strcpy(lamp.addr, optarg);
	break;
      case 'n':
        dev_name = optarg;
        break;
      default: /* '?' */
	arg_error = true;
    }
  }
  if (optind < argc) {
    fprintf(stderr, "Unknown argument %s\n", argv[optind]);
    arg_error = true;
  }
  if (!addr || !port || arg_error) {
    fprintf(stderr, "Usage: %s -a <addr> -p <port> [-h <hops>] [-u <uuid>]\n",
	    argv[0]);
    exit(EXIT_FAILURE);
  }


  /* Join the xAAL bus */
  if ( !xAAL_join_bus(addr, port, hops, 1, &bus) )
    exit(EXIT_FAILURE);

  /* Generate device address if needed */
  if ( uuid_is_null(uuid) ) {
    uuid_generate(uuid);
    uuid_unparse(uuid, lamp.addr);
    printf("Device: %s\n", lamp.addr);
  }


  /* Manage alive notifications */
  act_alarm.sa_handler = alive_sender;
  act_alarm.sa_flags = ~SA_RESETHAND;
  sigemptyset(&act_alarm.sa_mask);
  sigaction(SIGALRM, &act_alarm, NULL);
  alarm(ALIVE_PERIOD);

  if ( !xAAL_notify_alive(&bus, &lamp) )
    fprintf(xAAL_error_log, "Could not send initial alive notification.\n");

//printf("childrens: %s\n", json_object_get_string(xAAL_vector2array(lamp.childrens)));
  /* Main loop */
  for (;;) {

    /* Show lamp */
    if (attribute_light)
      printf("%s (O) On \r", dev_name);
    else
      printf("%s  .  Off\r", dev_name);
    fflush(stdout);

    /* Recive a message */
    if (!xAAL_read_bus(&bus, &jmsg, &version, &source, &jtargets, &msgType,
		  &devType, &action, &cipher, &signature, &timestamp))
      continue;

    if ( !xAAL_targets_match(jtargets, lamp.addr) ) {
      /* This is not for me */
      json_object_put(jmsg);
      continue;
    }

    if (strcmp(msgType, "request") == 0) {
      if ( (strcmp(action, "isAlive") == 0)
	  && xAAL_isAliveDevType_match(jmsg, lamp.devType) ) {
	if ( !xAAL_notify_alive(&bus, &lamp) )
	  fprintf(xAAL_error_log, "Could not reply to isAlive\n");

      } else if ( strcmp(action, "getDescription") == 0 ) {
	if ( !xAAL_reply_getDescription(&bus, &lamp, source) )
	  fprintf(xAAL_error_log, "Could not reply to getDescription\n");

      } else if ( strcmp(action, "getAttributes") == 0 ) {
	/* I ignore this */

      } else if ( strcmp(action, "getBusConfig") == 0 ) {
	if ( !xAAL_reply_getBusConfig(&bus, &lamp, source) )
	  fprintf(xAAL_error_log, "Could not reply to getBusConfig\n");

      } else if ( strcmp(action, "setBusConfig") == 0 ) {
	if ( !xAAL_reply_setBusConfig(&bus, &lamp, source) )
	  fprintf(xAAL_error_log, "Could not reply to setBusConfig\n");

      } else if ( strcmp(action, "getCiphers") == 0 ) {
	if ( !xAAL_reply_getCiphers(&bus, &lamp, source) )
	  fprintf(xAAL_error_log, "Could not reply to getCiphers\n");

      } else if ( strcmp(action, "setCiphers") == 0 ) {
	if ( !xAAL_reply_setCiphers(&bus, &lamp, source) )
	  fprintf(xAAL_error_log, "Could not reply to setCiphers\n");

      } else if ( strcmp(action, "on") == 0 ) {
	  attribute_light = true;

      } else if ( strcmp(action, "off") == 0 ) {
	    attribute_light = false;
      }
    }
    json_object_put(jmsg);

//printf("childrens: %s\n", json_object_get_string(xAAL_vector2array(lamp.childrens)));
//fflush(stdout);
  }
}
