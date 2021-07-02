# panelmd
panel de pon for mega drive - SGDK 1.62

***todo:***

check Hori+Vert matches after gravity

===========

vscroll to raise stack rather than pop up a new row of tiles - smoother

===========

when calling for checkmatch from pressing A or C, let's check both of the surrounding colors to see if either is the same, if not don't need to check
so if you are switching tiles 3,3 and 4,3, check 2,3's color and 5,3's color before doing their hori match checking

===========

does the hori matching take into account 6 across from a grav drop?
I think so - just checkmatching after gravity isn't yet implemented

===========

dont allow switching tiles while checking is active

===========

convert falling pieces to sprites so I can make them fall 1px at a time

===========

in gravity, each piece hangs in the air for a little while, then falls
LOTS of time for calculations

when a piece is destroyed, it stays there, it doesn't disappear automatically
change palette on the destroying pieces

===========

1) connection is made [DONE]
2) tile status is changed so that it cannot be swapped [DONE]
3) graphic is immediately changed for all connected tiles immediately for Y time (FLASHING) [DONE]
4) graphic is changed to disappearing graphic for Y time
5) tiles are made invisible and removed from the board

===========

update/fix timer function
4 timers
at check, look at each of the 4
if any of them aren't 0 (their value will be when the timer event happened), check them
when they reach 60ms or whatever the break, do what their timer event needs to do and set it to zero

each timer needs to have an array of tiles that get destroyed when it hits 0
maybe the destroyindex stuff shouldn't get sent to 0
each destroy has a length and the info is saved in destroyindex
so we need a start and a length and that'll give us which tiles get destroyed at that timer event time

===========

destroyeventsIndex can be u8 not u16, need to reset it back to 0 if all 4 events are unused
