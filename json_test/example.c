#include <json/json.h>
#include <stdio.h>

/*printing the value corresponding to boolean, double, integer and strings*/
void print_json_value(json_object *jobj){
  enum json_type type;
  printf("type: ",type);
  type = json_object_get_type(jobj); /*Getting the type of the json object*/
  switch (type) {
    case json_type_boolean: printf("json_type_booleann");
                         printf("value: %sn", json_object_get_boolean(jobj)? "true": "false");
                         break;
    case json_type_double: printf("json_type_doublen");
                        printf("          value: %lfn", json_object_get_double(jobj));
                         break;
    case json_type_int: printf("json_type_intn");
                        printf("          value: %dn", json_object_get_int(jobj));
                         break;
    case json_type_string: printf("json_type_stringn");
                         printf("          value: %sn", json_object_get_string(jobj));
                         break;
  }

}

void json_parse_array( json_object *jobj, char *key) {
  void json_parse(json_object * jobj); /*Forward Declaration*/
  enum json_type type;

  json_object *jarray = jobj; /*Simply get the array*/
  if(key) {
    jarray = json_object_object_get(jobj, key); /*Getting the array if it is a key value pair*/
  }

  int arraylen = json_object_array_length(jarray); /*Getting the length of the array*/
  printf("Array Length: %dn",arraylen);
  int i;
  json_object * jvalue;

  for (i=0; i< arraylen; i++){
    jvalue = json_object_array_get_idx(jarray, i); /*Getting the array element at position i*/
    type = json_object_get_type(jvalue);
    if (type == json_type_array) {
      json_parse_array(jvalue, NULL);
    }
    else if (type != json_type_object) {
      printf("value[%d]: ",i);
      print_json_value(jvalue);
    }
    else {
      json_parse(jvalue);
    }
  }
  /* to be modified */
  int i, len;
  json_object record;
  json_object jtargets;
  len = json_object_array_length(jtargets);
  printf("Array Length: %d\n",len);

  for (i = 0; i < len; i++) {
      record = json_object_array_get_idx(jtargets, i);
      if (json_object_get_type(record) == json_type_object) {
            target = json_object_get_string(jtarget);
            if (strcmp(target, addr) == 0)
              return true;
      }
  }
}

/*Parsing the json object*/
void json_parse(json_object * jobj) {
  enum json_type type;
  json_object_object_foreach(jobj, key, val) { /*Passing through every array element*/
    printf("type: ",type);
    type = json_object_get_type(val);
    switch (type) {
      case json_type_boolean:
      case json_type_double:
      case json_type_int:
      case json_type_string: print_json_value(val);
                           break;
      case json_type_object: printf("json_type_objectn");
                           jobj = json_object_object_get(jobj, key);
                           json_parse(jobj);
                           break;
      case json_type_array: printf("type: json_type_array, ");
                          json_parse_array(jobj, key);
                          break;
    }
  }
}

int main() {
  char * string = "{"sitename" : "joys of programming",
                     "categories" : [ "c" , ["c++" , "c" ], "java", "PHP" ],
                     "author-details": { "admin": false, "name" : "Joys of Programming", "Number of Posts" : 10 }
                     }";
  printf("JSON string: %sn", string);
  json_object * jobj = json_tokener_parse(string);
  json_parse(jobj);
}
