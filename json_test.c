#include <stdio.h>
#include <json-c/json.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <uuid/uuid.h>
#include <string.h>
#include <unistd.h>

int main() {
  struct json_object *jbody;
  struct json_object *jmsg, *jheader;
  const char *msg;
  int ret;
  uuid_t uuid1, uuid2;
  char addr1[37];
  char addr2[37];
  int fd;
  char *fname = "json_output_array.txt";
  
  uuid_clear(uuid1);
  uuid_clear(uuid2);
  
  if (uuid_is_null(uuid1)) {
    uuid_generate(uuid1);
    uuid_unparse(uuid1, addr1);
    printf("Device1: %s\n", addr1);
  }
  if (uuid_is_null(uuid2)) {
      uuid_generate(uuid2);
      uuid_unparse(uuid2, addr2);
      printf("Device2: %s\n", addr2);
    }
  
#if 0  
  jbody = json_object_new_object();
  json_object_object_add(jbody, "timeout",
			 json_object_new_int64(20));


  jheader = json_object_new_object();
  json_object_object_add(jheader, "version",   json_object_new_string("0.4"));
  json_object_object_add(jheader, "targets",   json_object_new_array());
  json_object_object_add(jheader, "source",    json_object_new_string(addr));
  json_object_object_add(jheader, "devType",   json_object_new_string("hmi.basic"));
  json_object_object_add(jheader, "msgType",   json_object_new_string("notify"));
  json_object_object_add(jheader, "action",    json_object_new_string("alive"));
  json_object_object_add(jheader, "cipher",    json_object_new_string("none"));
  json_object_object_add(jheader, "signature", json_object_new_string(""));

  jmsg = json_object_new_object();
  json_object_object_add(jmsg, "header", jheader);
  if (jbody)
    json_object_object_add(jmsg, "body", jbody);
#else
  jbody = json_object_new_object();
  json_object_object_add(jbody, "uuid", json_object_new_string(addr1));
  json_object_object_add(jbody, "dev_type", json_object_new_string("lamp"));
  json_object_object_add(jbody, "location", json_object_new_string("kitchen"));
  
  jheader = json_object_new_object();
  json_object_object_add(jheader, "uuid", json_object_new_string(addr2));
  json_object_object_add(jheader, "dev_type", json_object_new_string("lamp"));
  json_object_object_add(jheader, "location", json_object_new_string("livingroom"));
  jmsg = json_object_new_array();
  json_object_array_add(jmsg, jbody);
  json_object_array_add(jmsg, jheader);
  
  
#endif
  
  msg = json_object_to_json_string_ext(jmsg, JSON_C_TO_STRING_PLAIN|JSON_C_TO_STRING_NOZERO);

  fd = open(fname, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd < 0) {
      perror("open");
      json_object_put(jmsg);
      return -1;
  }
  if (write(fd, msg, strlen(msg)) < 0) {
      perror("write");
      json_object_put(jmsg);
      return -1;
  }


  json_object_put(jmsg);
  close(fd);
  return ret;
  
}