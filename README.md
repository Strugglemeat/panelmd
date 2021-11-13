cursor should be animated - pulses (middle section only moves Y+/Y-, outer sections move both. they are all the same tile, flipped)
===
it's deleting extra tiles on match - need to cleanup the destroy array
===
destroying: T and L combos don't work
===
work on the redraw function to redraw less of the screen after gravity and any other times when it does a full screen redraw
==
work on the gravity function to use the parameter and calc less, even just half the board would be an improvement
===
connection checks after gravity dont work if it is a hori that was dropped 1 tile height
===
generate new rows - still no vertical matching check
===
send up rows while tiles destroying - buggy (maybe this should just be totally disabled - because in 2p they shouldnt be able to raise while someone is working?)
===
needs pause midair when a piece falls due to gravity - not instant
dogravity now being called EVERY TIME we match a piece - not necessary
specify min and max X coords for doGravity(), cuts down a lot
===
convert gravity falling pieces to sprites so they can fall 1px at a time
===
vertical checkmatch goes through an entire column, not efficient
whatever gets connected MUST have touched the switched tiles (what about gravity?)
===
modes:
endless
time trial
story progression (enemy 'freezes' certain tiles at certain times etc)

options:
FPS on/off
colors 4 thru 6
music on/off
===
palettes
3-tiles
2-bg
1-?
0-?
