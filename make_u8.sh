#!/bin/bash
if [ -z "$1" ]
then
    echo "type in filename to convert to bitbox audio (w8v)"
    echo "add a second argument to just convert to wav"
    exit 1
fi

filename=$(basename "$1")
extension="${filename##*.}"
filename="${filename%.*}"

if [ "$extension" == "wav" ]
then
    directory=$(dirname "$1")
    directory="${directory/#\~/$HOME}"
    directory=$(readlink -m "$directory")
    pwdir=$(pwd)
    if [ "$directory" == "$pwdir" ]
    then
        ffmpeg -i "$1" -acodec pcm_u8 -ac 2 -af "volume=1.3"  -ar 32000 "${filename}888.wav"
        if [ -z "$2" ]
        then
            _u8.py "${filename}888.wav" "${filename}.w8v"
            rm "${filename}888.wav"
        else
            mv "${filename}888.wav" "$2"
        fi
        exit 0
    fi
fi
ffmpeg -i "$1" -acodec pcm_u8 -ac 2 -af "volume=1.3"  -ar 32000 "${filename}.wav"
if [ -z "$2" ]
then
    _u8.py "${filename}.wav" "${filename}.w8v"
    rm "${filename}.wav"
else
    mv "${filename}.wav" "$2"
fi


