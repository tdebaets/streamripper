README for xfade processing patch 2
===================================
Greg Sharp
gregsharp@geocities.com

The new track separation method has the following parameters that you 
can tweak.  All times are in milliseconds.

--xs_silence_length=num
                The volume must be less than xsd_min_volume for a period 
                of time greater than this.

--xs_search_window=num:num
                This is how long to search for the silence.  1st number 
                is # msec before nominal center, 2nd number is # msecs after
                nominal track change position.

--xs_offset=num
                Offset from center of silence window.

--xs_padding=num:num
                Amount to pad before and after splitpoint.

Default splitting
-----------------
The default spitting algorithm is used when no silent point can be found.
Suppose you have a meta-int with track change information at the time "mi"
(see figure below).

If the xs_offset is positive, the track separation point "ts" is later
the "mi" point.  If xs_offset is negative, "ts" is earlier than "mi".
Once "ts" is determined, a user-defined "prepad" and "postpad" 
are used to determine where the next track begins "ntb", and where
the previous track ends "pte".  The interval between "ntb" and "pte"
will be copied to both songs.

            /mi
            |
            |           /ts
            |-----------|
              xs_offset |
                        |
                        |
              /ntb      |         /pte
              |---------|---------|
                prepad    postpad


Silence separation
------------------
Splitting based on silence separation is similar to default splitting,
only slightly more complex.  Again, suppose you have a meta-int 
with track change information at the time "mi" (see figure below).

A search window "search_win" is determined by the xs_offset, pre_sw, 
and post_sw field.  The beginning of the search window is at:
     mi + xs_offset - pre_sw
and the end of the search window is at:
     mi + xs_offset + post_sw.

If there is a silent interval of length "silence_win" within the 
"search_win", the center of "silence_win" is selected as the 
track separation point "ts".

Once "ts" is determined, a user-defined "prepad" and "postpad" 
are used to determine where the next track begins "ntb", and where
the previous track ends "pte".  The interval between "ntb" and "pte"
will be copied to both songs.

            /mi
            |  
            |-----------|
              xs_offset |
                        |
                    ts\ |
              |-------+-|---------| *search_win
               pre_sw |   post_sw
                      |
                  |---+---| *silence_win
                      |
        /ntb          |         /pte
        |-------------|---------|
              prepad    postpad



Usage examples
--------------
1) Each of my songs contain about 5 seconds of the previous song.
How can I fix this?

./streamripper URL --xs_offset=5000

2) Each of my songs contain about 5 seconds of the next song.  
How can I fix?

./streamripper URL --xs_offset=-5000

3) Each of my songs contain between 5 and 10 seconds of the previous 
song, but it depends on the song.  I want to include all of this 
zone within both songs, and edit them later.

./streamripper URL --xs_offset=7500 --xs_padding=2500:2500

or

./streamripper URL --xs_offset=5000 --xs_padding=0:5000


Future plans
------------
Here are some ideas that are still not implemented.

Between tracks, there will always be a splitpoint selection phase.
Cue sheets will be indexed according to the selected splitpoint.
If file output is selected, the splitpoint will be used as the 
separation point, but padding may be optionally added.

I really like the single song/cue sheet option.  Makes a lot of 
sense for streams that don't separate easily.

-x1  File output
-x2  Cue sheet output
-x3  Cue sheet and file output

Also, like to have a minimum volume for splitting

--xs_min_volume=num
                The volume must be less than this to be considered silence.
