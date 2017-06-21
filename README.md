# Speech Recognition
This project is an stt module based on PocketSphinx

**For hello\_world program, pocketsphinx takes as input an audio file with following requirements**

Record an audio file: signed 16-bit, little-endian, sample rate 16kHz, single channel  
*arecord -r 16000 -f S16_LE -c 1 -D plughw:0,0 -t raw test_hw.raw*  
Play an audio file: signed 16-bit, little-endian, sample rate 16kHz  
*aplay -f S16_LE -r 16000 test_hw.raw*

In order to compile json\_parser and json\_test  
*gcc json\_parser.c -I/usr/include/uuid -I/usr/include/json-c -ljson-c -o json_parser*  
  
To do the demo for lamp control  
*./voice\_commandor -inmic yes -lm model\_with\_keyword/model\_kwd.lm -dict model\_with\_keyword/model\_kwd.dic -keyphrase "OBAMA" -kws_threshold 1e-5 -logfn log.log*  

1st lamp  
*./dummyLamp -a 224.0.29.200 -p 1234 -u 1b51f38e-ae97-41d0-a08d-297f68ed0be9*  
  
2nd lamp  
*./dummyLamp -a 224.0.29.200 -p 1234 -u 9b6b19b3-a213-4bee-865a-dba40a4472e1*  
  
Location tags are configured in *device\_config.json*  
  
**Source Code Tree**  
- ad\_acous\_module and ad\_acous\_module are two trained acoustic modules. They are of objective to show how to train an acoustic module. The difference is in ad\_acous\_module, I added 6 times the same phrase "turn on the light" in order to improve the recognition rate for the word "light". But as a result, it didn't seem to improve a lot.  
- json\_test contains some practical examples for creating json object or parsing json object.  
- model\_with\_keyword is the final language model we trained and used since "portes ouvertes".  
- ss contains the screen shots of system performance while our program is running on a raspberry pi v3.  
- .asoundrc is an example for ALSA configuration.  
- .project is the project file under Eclipse.  
- corpus_file.txt is the corpus file that we used to construct the final language model.  
- device_config.json is the device list we mentioned in the technical report.  
- dummyLamp.c is the dummyLamp application developped by xAAL team.  
- dummy_commander.c is the interaction module.  
- smtc_module.c is the semantic analysis module.  
- stt_module.c is the STT module.  
- smtc_module.h contains the data structures and interfaces shared between semantic analysis module and interaction module.
