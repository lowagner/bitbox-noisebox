# noisebox
another music thingy for the bitbox.

The first thing it does (and the last thing) is make white noise (closer to red).

But if you hit the user button, this program will play a songlist of `MAX_FILES`
(set to 64, but you can adjust).  The next time it boots, it chooses the next set
by jumping forward `MAX_FILES/2`, but FAT filesystems don't necessarily put 
things in alphabetical order.  (See [Fat Sort](http://fatsort.sourceforge.net/) 
for a solution.)  Nevertheless, the noisebox program will alphabetically sort 
the files that it does find, so at least some order is restored.

## Controls

Only controllable via the bitbox user button (middle button on the bitbox itself).

Tap the user button quickly to advance forward one song.  Hold down the user
button to seek forward in the current song.  Hit reset to advance a few songs
and get some new songs in the playlist.

## Add music to the library

run `make_u8.sh "Name of your file.mp3"` (or whatever extension),
and you can create the `w8v` format valid for the bitbox decoder here.
Just put that on the SD card and the noisebox program will find it.
