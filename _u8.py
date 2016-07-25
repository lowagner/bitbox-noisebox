#!/usr/bin/env python2
import wave, sys, struct

if len(sys.argv) != 3:
    print "need 2 arguments:  wav file and out file"
    sys.exit(1)

wavfile = wave.open(sys.argv[1],'rb')
assert wavfile.getframerate()==32000, 'wrong framerate'
assert wavfile.getnchannels()==2, 'only dual channel'
assert wavfile.getsampwidth()==1, 'only 8bits'

with open(sys.argv[2],'wb') as of: 
    while True:
        samples = wavfile.readframes(1066)
        for s in samples:
            of.write(s)
        if len(samples) < 2132:
            break 
