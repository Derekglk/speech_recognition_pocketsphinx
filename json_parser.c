#include <json-c/json.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#define MAX_CONFIG_SIZE (1024)
#define CONFIG_FILE "device_config.json"

int json_parse_config(json_object *jobj) {
  int i, len;
  int ret = 0;
  json_object *record, *juuid, *jtype, *jlocation;
  const char *uuid, *type, *location;

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
	  uuid = json_object_get_string(juuid);
	  type = json_object_get_string(jtype);
	  location = json_object_get_string(jlocation);

	  printf("uuid = %s\n", uuid);
	  printf("type = %s\n", type);
	  printf("location = %s\n", location);
      } else {
	  printf("config list element is NOT object\n");
	  ret = -1;
      }
  }
  return ret;
}





int main() {
  int fd;
  char config[MAX_CONFIG_SIZE];
  struct stat fd_stat;
  json_object *jobj;
  struct json_tokener *tok;
  ssize_t nread;
  json_type type;
  enum json_tokener_error jerr;

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

  switch (type) {
    case json_type_boolean:
      printf("json_type_boolean\n");
      break;
    case json_type_double:
      printf("json_type_double\n");
      break;
    case json_type_int:
      printf("json_type_int\n");
      break;
    case json_type_string:
      printf("json_type_string\n");
      break;
    case json_type_object:
      printf("json_type_object\n");
      break;
    case json_type_array:
      printf("json_type_array\n");
      break;
    default:
      printf("unknown type\n");
      break;

  }
  json_parse_config(jobj);

  return 0;
}
