make pulling up a new row make it come even further up than needed and then fall back down with a slam 
-------
sprite-related bug with swapping that makes all tiles disappear
-------
refactor renderScene() to work with both players
--------
bugginess with quickly swapping
---------
make it so the cursors cannot overlap
---------
cycle palette 2 to make background "animate"
-----
rise stack while match going off messes it up
--------
reduce gravity falling speed - introduce delay
--------
[graphical flourish]
explosion anim at each block that disappears
--------
[gameplay]
recognition of chains
chain definition - a falling piece is used in a combo
--------
fix the switchsprite wonkiness
*do i need to create one at every swap?
--------
[optimization]
work on the redraw function to redraw less of the screen after gravity and any other times when it does a full screen redraw
--------
[optimization]
work on the gravity function to restrict X , even just half the board would be an improvement
specify min and max X coords for doGravity(), cuts down a lot
--------
[optimization]
dogravity now being called EVERY TIME we match a piece - try to restrict it more
--------
convert gravity falling pieces to sprites so they can fall 1px at a time
make them bounce a bit - let it fall PAST the destination, then come back
--------
[optimization]
vertical checkmatch goes through an entire column, not efficient
whatever gets connected MUST have touched the switched tiles (what about gravity?)
--------
modes:
endless
time trial
story progression (enemy 'freezes' certain tiles at certain times etc)
mission attack

options:
FPS on/off
colors 4 thru 6
music on/off
--------
palettes
3-tiles (6x2 = 12, 3 for explosions, 1 for transparency)
2-bg & cursors
1-?
0-?
--------=
mission attack
x? combo
x? chain
allclear
allfull (no empty spaces)
destroy specific marked tiles (ala columns)

maybes:
--------
hori double pieces?
--------
puyo/pnickies-style blob matching graphics
