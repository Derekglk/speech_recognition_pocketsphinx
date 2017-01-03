# tts_module
This project is an tts module based on PocketSphinx

**For hello\_world program, pocketsphinx takes as input an audio file with following requirements**

Record an audio file: signed 16-bit, little-endian, sample rate 16kHz, single channel
*arecord -r 16000 -f S16_LE -c 1 -D plughw:0,0 -t raw test_hw.raw*
Play an audio file: signed 16-bit, little-endian, sample rate 16kHz
*aplay -f S16_LE -r 16000 test_hw.raw*

