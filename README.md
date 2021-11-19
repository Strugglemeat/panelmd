recognition of chains
===
fix the cursor wonkiness
*do i need to create one at every swap?
===
cursor should be animated - pulses (middle section only moves Y+/Y-, outer sections move both. they are all the same tile, flipped)
===
migrate stack raise from 1 full row at a time to scrolling
===
work on the redraw function to redraw less of the screen after gravity and any other times when it does a full screen redraw
==
work on the gravity function to restrict X , even just half the board would be an improvement
specify min and max X coords for doGravity(), cuts down a lot
===
send up rows while tiles destroying - buggy (maybe this should just be totally disabled - because in 2p they shouldnt be able to raise while someone is working?)
===
dogravity now being called EVERY TIME we match a piece - try to restrict it more
===
convert gravity falling pieces to sprites so they can fall 1px at a time
make them bounce a bit - let it fall PAST the destination, then come back
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
