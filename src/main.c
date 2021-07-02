#include <genesis.h>
#include <resources.h>

#define maxX 6
#define maxY 12
#define blocksize 16
#define p1boardstartX 16
#define p2boardstartX 208
#define boardstartY 16

typedef struct {
Sprite* cursor;
u8 cursorX,cursorY;
u8 xpos,ypos;

u8 moveDelay,acceleration;
u16 lastDirInput;
u8 lastDirInputHowMany;
u8 raiseDelay;

u8 destroyIndex;
u8 destroyX[17];
u8 destroyY[17];//max 16 destroyed in one move - 0 of index isn't used

u8 board[maxX+2][maxY+2];//6x12 playfield, with room around the sides. start at [1][1] for bottom left corner

u8 hasSwitched;

u8 flag_redrawAll;
} Player;

Player p1;

enum tile{Red=1, Purple=2, Yellow=3, Green=4, Blue=5, Darkblue=6};

#define moveDelayAmt 8
#define MAX_SPEEDUP 6//this can't be more than moveDelayAmt!
#define accelerationPace 4//lower is faster
#define accelerationAmount 2//higher is faster

#define raiseDelayAmount 12//lower is faster

static void drawBorder();
static void handleInput();
static void updateSprites();
static void print_debug();
static u8 randomRange(u8 rangeStart, u8 rangeEnd);
//static void updateBackground(u8 yStart, u8 yEnd);
static void updateBackground();
static void drawTile(u8 x, u8 y, u8 color);
static void insertInitialRowData();
static void generateNewRow();
static void pushupRows();
static u8 checkTopRow();
static void doGravity(u8 highestRow);
static void clearGrid();
static void checkMatchRow(u8 whichRow, u8 color1, u8 color2);
static void checkMatchColumn(u8 whichColumn, u8 color);
static void destroyMatchedTiles();
//static void queuedBackgroundUpdate(u8 updateRowBegin, u8 updateRowEnd);
static void queuedBackgroundUpdate();
static void renderScene();
static void processDestroy();
static void checkTimer();

#define borderIndex 1
#define tileIndex 5

u8 numColors=5;//6 includes the dark blue tile

char debug_string[40] = "";

//u8 flag_emptySwitch=0;
u8 flag_gravityErase=0;

s16 timer=0;
s16 timerEvents[4];
u8 timerEventsIndex=0;

#define destructionTime 60

int main()
{
SYS_disableInts();

VDP_setScreenWidth320();
VDP_setScreenHeight224();
VDP_loadFontData(tileset_Font_Namco.tiles, 96, CPU);
VDP_setPalette(PAL1, cursor.palette->data);

//border
//VDP_setPalette(PAL2, bgtile.palette->data);
VDP_loadTileSet(bgtile.tileset,borderIndex,DMA);

//tiles
VDP_setPalette(PAL3, alltiles.palette->data);
VDP_loadTileSet(alltiles.tileset,tileIndex,DMA);
VDP_setPalette(PAL2, modtiles.palette->data);

drawBorder();

clearGrid();

insertInitialRowData();
updateBackground();

sprintf(debug_string,"press B");
VDP_drawText(debug_string,3,8);
sprintf(debug_string,"to raise");
VDP_drawText(debug_string,3,9);
sprintf(debug_string,"the stack");
VDP_drawText(debug_string,3,10);

SYS_enableInts();

SPR_init();
p1.cursor = SPR_addSprite(&cursor,0,0,TILE_ATTR(PAL1,0,FALSE,FALSE));
SPR_setVisibility(p1.cursor,HIDDEN);//make it hidden while doing loading/init stuff

p1.xpos=1;
p1.ypos=maxY;
p1.cursorX=p1boardstartX+((p1.xpos-1)*blocksize)-2;
p1.cursorY=boardstartY+((p1.ypos-1)*blocksize)-2;

SPR_setVisibility(p1.cursor,VISIBLE);
updateSprites();//has SPR_Update() in it

p1.flag_redrawAll=0;

	while(1)
	{
		timer++;
		handleInput();
		if(p1.destroyIndex>=3)processDestroy();
		if(timerEventsIndex>0)checkTimer();
		SYS_doVBlankProcess();
		renderScene();
		print_debug();
	}

	return(0);
}

static void drawBorder()
{
	u8 iX,iY;
	for(iX=0;iX<40;iX+=2)//top and bottom rows
	{
		VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, borderIndex), iX, 0, 2, 2);
		VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, borderIndex), iX, 26, 2, 2);
	}

	for(iY=2;iY<25;iY+=2)//left and right sides, inner walls
	{
		VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, borderIndex), 0, iY, 2, 2);
		VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, borderIndex), 38, iY, 2, 2);

		VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, borderIndex), 14, iY, 2, 2);
		VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, borderIndex), 24, iY, 2, 2);
	}
}

static void handleInput()//can be held down
{
	u16 value1 = JOY_readJoypad(JOY_1);

	if(value1==p1.lastDirInput && value1!=0)p1.lastDirInputHowMany++;//count for acceleration
	else if(value1!=p1.lastDirInput){p1.lastDirInputHowMany=0;p1.acceleration=0;}//if changing directions
	else if(value1==0){p1.lastDirInput=0;p1.lastDirInputHowMany=0;p1.acceleration=0;}//if no more dpad input

	if(p1.lastDirInputHowMany%accelerationPace==0 && p1.lastDirInputHowMany>accelerationPace)p1.acceleration+=accelerationAmount;
	if(p1.acceleration>MAX_SPEEDUP)p1.acceleration=MAX_SPEEDUP;

	if(p1.moveDelay>0)p1.moveDelay--;
	else if(p1.moveDelay==0)
	{
	    if ((value1 & BUTTON_LEFT) && p1.xpos>1)   {p1.xpos--;p1.moveDelay=moveDelayAmt-p1.acceleration;p1.lastDirInput=value1;p1.cursorX-=blocksize;updateSprites();}
	    if ((value1 & BUTTON_RIGHT) && p1.xpos < maxX-1)   {p1.xpos++;p1.moveDelay=moveDelayAmt-p1.acceleration;p1.lastDirInput=value1;p1.cursorX+=blocksize;updateSprites();}
	    if ((value1 & BUTTON_UP) && p1.ypos>1)      {p1.ypos--;p1.moveDelay=moveDelayAmt-p1.acceleration;p1.lastDirInput=value1;p1.cursorY-=blocksize;updateSprites();}//1,1 is the bottom left...
	    if ((value1 & BUTTON_DOWN) && p1.ypos<maxY)    {p1.ypos++;p1.moveDelay=moveDelayAmt-p1.acceleration;p1.lastDirInput=value1;p1.cursorY+=blocksize;updateSprites();}
   	}

	if(p1.raiseDelay>0)p1.raiseDelay--;
	else if (p1.raiseDelay==0)
	{
		if(value1 & BUTTON_B){if(checkTopRow()==0)generateNewRow();p1.raiseDelay=raiseDelayAmount;}
	}

	if(value1 & BUTTON_START)doGravity(0);

	if(((value1 & BUTTON_A) || value1 & BUTTON_C) && p1.hasSwitched==0)
	{
		p1.hasSwitched=1;

		u8 color1=0,color2=0;
        color1=p1.board[p1.xpos][p1.ypos];
        color2=p1.board[p1.xpos+1][p1.ypos];//this is to see if we swapped with an empty, to do gravity

		if(color1<=numColors && color2<=numColors)
		{
			p1.board[p1.xpos][p1.ypos]=p1.board[p1.xpos+1][p1.ypos];
			p1.board[p1.xpos+1][p1.ypos]=color1;

			p1.flag_redrawAll=1;

			checkMatchRow(p1.ypos,color1,color2);
			checkMatchColumn(p1.xpos,color2);
			checkMatchColumn(p1.xpos+1,color1);
		}


		if(color1==0 || color2==0)//empty swap
			{
				p1.flag_redrawAll=1;
				//flag_emptySwitch=1;//set this flag ON when we did an empty swap - otherwise we don't need to clear the tiles we swapped
				//doGravity(p1.ypos-1);//this only sends downwards, not upwards. would be appropriate for switching if there is nothing on top
				doGravity(0);//this does the whole board
			}
	}

	if((!(value1 & BUTTON_A)) && (!(value1 & BUTTON_C))){p1.hasSwitched=0;}

}

//static void updateBackground(u8 yStart, u8 yEnd)
static void updateBackground()
{
	u8 iX,iY;

	u8 yStart=1;//1 is the topmost row
	u8 yEnd=maxY+1;//maxY+1 is the bottom row

	for (iX=1;iX<maxX+1;iX++)
		{
		for (iY=yStart;iY<yEnd;iY++)//goes from the top down. 1,12 is bottom left, 1,1 is top left.
			{						  //a lower number means drawing starting from lower on the board.
			if(p1.board[iX][iY]!=0)
				{
				drawTile(iX,iY,p1.board[iX][iY]);
				}
			}

		}
}

static void updateSprites()
{
    SPR_setPosition(p1.cursor,p1.cursorX,p1.cursorY);
	SPR_update();
}

static void print_debug()
{
	sprintf(debug_string,"FPS:%ld", SYS_getFPS());
	VDP_drawText(debug_string,34,0);

	sprintf(debug_string,"player1");
	VDP_drawText(debug_string,16,2);

	sprintf(debug_string,"crsrX:%d",p1.xpos);
	VDP_drawText(debug_string,16,3);

	if(p1.ypos<10)VDP_clearText(23,4,1);
	sprintf(debug_string,"crsrY:%d",p1.ypos);
	VDP_drawText(debug_string,16,4);

	sprintf(debug_string,"accel:%d",p1.acceleration);
	VDP_drawText(debug_string,16,5);

	sprintf(debug_string,"timer:%d",timer);
	VDP_drawText(debug_string,16,6);

	if(p1.destroyIndex>=3)
	{
	//VDP_clearText(14,7,10);
	sprintf(debug_string,"p1Dstry:%d",p1.destroyIndex);
	VDP_drawText(debug_string,16,7);
	}

	sprintf(debug_string,"tmrEvent0:%d",timerEvents[0]);
	VDP_drawText(debug_string,16,8);

	sprintf(debug_string,"tmrEvent1:%d",timerEvents[1]);
	VDP_drawText(debug_string,16,9);

	sprintf(debug_string,"tmrEvent2:%d",timerEvents[2]);
	VDP_drawText(debug_string,16,10);

	sprintf(debug_string,"tmrEvent3:%d",timerEvents[3]);
	VDP_drawText(debug_string,16,11);

}

static u8 randomRange(u8 rangeStart, u8 rangeEnd)
{
	return (random() % (rangeEnd + 1 - rangeStart)) + rangeStart;
}

static void drawTile(u8 x, u8 y, u8 color)
{
	switch(color)
	{//enum tile{Red=1, Purple=2, Yellow=3, Green=4, Blue=5, Darkblue=6};
		case Red:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+color-1), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+1+color-1), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+12+color-1), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+13+color-1), x+x+1, y+y+1, 1, 1);
		break;

		case Purple:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+color), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+1+color), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+12+color), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+13+color), x+x+1, y+y+1, 1, 1);
		break;

		case Yellow:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+color+1), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+1+color+1), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+12+color+1), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+13+color+1), x+x+1, y+y+1, 1, 1);
		break;

		case Green:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+color+2), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+1+color+2), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+12+color+2), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+13+color+2), x+x+1, y+y+1, 1, 1);
		break;

		case Blue:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+color+3), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+1+color+3), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+12+color+3), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+13+color+3), x+x+1, y+y+1, 1, 1);
		break;

		case Darkblue:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+color+4), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+1+color+4), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+12+color+4), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+13+color+4), x+x+1, y+y+1, 1, 1);
		break;

		case Red+6:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+color-1-6), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+1+color-1-6), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+12+color-1-6), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+13+color-1-6), x+x+1, y+y+1, 1, 1);
		break;

		case Purple+6:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+color-6), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+1+color-6), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+12+color-6), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+13+color-6), x+x+1, y+y+1, 1, 1);
		break;

		case Yellow+6:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+color+1-6), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+1+color+1-6), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+12+color+1-6), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+13+color+1-6), x+x+1, y+y+1, 1, 1);
		break;

		case Green+6:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+color+2-6), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+1+color+2-6), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+12+color+2-6), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+13+color+2-6), x+x+1, y+y+1, 1, 1);
		break;

		case Blue+6:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+color+3-6), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+1+color+3-6), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+12+color+3-6), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+13+color+3-6), x+x+1, y+y+1, 1, 1);
		break;

		case Darkblue+6:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+color+4-6), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+1+color+4-6), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+12+color+4-6), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, tileIndex+13+color+4-6), x+x+1, y+y+1, 1, 1);
		break;

		case 255:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, 0), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, 0), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, 0), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL2, 0, FALSE, FALSE, 0), x+x+1, y+y+1, 1, 1);
		break;
	}
}

static void insertInitialRowData()
{
	//p1board[1][1]=2;
	//p1board[2][1]=3;
	//p1board[3][1]=4;
	//p1board[4][1]=5;
	//p1board[5][1]=6;

	p1.board[6][maxY-2]=3;
	p1.board[6][maxY-1]=2;
	p1.board[2][maxY]=2;
	p1.board[3][maxY]=1;
	p1.board[4][maxY]=2;
	p1.board[5][maxY]=1;
	p1.board[6][maxY]=1;
}

static void generateNewRow()
{
	for(u8 newX=1;newX<maxX+1;newX++)
	{
		p1.board[newX][maxY+1]=randomRange(1,numColors);
	}
//later on, change this to a do while, and do a check for matches in the new row
	if((p1.board[1][maxY+1]==p1.board[2][maxY+1]) && (p1.board[1][maxY+1]==p1.board[3][maxY+1])){p1.board[1][maxY+1]=1;p1.board[3][maxY+1]=2;}

	if((p1.board[4][maxY+1]==p1.board[5][maxY+1]) && (p1.board[4][maxY+1]==p1.board[6][maxY+1])){p1.board[4][maxY+1]=3;p1.board[6][maxY+1]=5;}

	if(p1.board[2][maxY+1]==p1.board[3][maxY+1] && p1.board[2][maxY+1]!=1)p1.board[3][maxY+1]=1;

	if(p1.board[4][maxY+1]==p1.board[5][maxY+1] && p1.board[4][maxY+1]<3)p1.board[5][maxY+1]=4;
	else if(p1.board[4][maxY+1]==p1.board[5][maxY+1] && p1.board[4][maxY+1]>=3)p1.board[5][maxY+1]=2;

	if(p1.board[3][maxY+1]==p1.board[4][maxY+1] && p1.board[3][maxY+1]<4)p1.board[4][maxY+1]=5;
	else if(p1.board[3][maxY+1]==p1.board[4][maxY+1] && p1.board[3][maxY+1]>=4)p1.board[4][maxY+1]=3;

	if(p1.board[5][maxY+1]==p1.board[6][maxY+1] && p1.board[5][maxY+1]>1)p1.board[5][maxY+1]=1;

	pushupRows();
}

static void pushupRows()
{
	for(u8 yPos=1;yPos<maxY+1;yPos++)
	{
		for(u8 xPos=1;xPos<maxX+1;xPos++)
		{
			p1.board[xPos][yPos]=p1.board[xPos][yPos+1];
		}
	}

	p1.flag_redrawAll=1;

	for(u8 x=1;x<maxX;x++)
	{
		if(p1.board[x][maxY]==p1.board[x][maxY-1]){checkMatchColumn(x, p1.board[x][maxY]);}
	}
}

static u8 checkTopRow()
{
	u8 emptyCheck=0;
		for(u8 xPos=1;xPos<maxX+1;xPos++)
		{
			if(p1.board[xPos][1]>0)emptyCheck=1;
		}

	if(emptyCheck>0)return 1;
	else return 0;
}

static void doGravity(u8 highestRow)//the parameter here should be the lowest (highest number) row of where the tile(s) landed
{
	u8 gravityDoneFlag=0;
	u8 hasGravity=0;
	//u8 saveColor=0,saveY=0;

	do
	{
	hasGravity=0;
		for(u8 gravY=maxY-1;gravY>highestRow;gravY--)
		{
			for(u8 gravX=1;gravX<maxX+1;gravX++)
			{
				if(p1.board[gravX][gravY]>0 && p1.board[gravX][gravY+1]==0)
				{
					p1.board[gravX][gravY+1]=p1.board[gravX][gravY];
					p1.board[gravX][gravY]=0;
					gravityDoneFlag=1;
					hasGravity++;
					
					
					//saveColor=p1.board[gravX][gravY+1];
					//saveY=gravY+1;
				}
			}
		}
	}while(hasGravity!=0);


	if(gravityDoneFlag>0)
		{
			p1.flag_redrawAll=1;
			//flag_gravityErase=1;//gravY >> 2;
			//updateBackground(1,maxY);
			//checkMatchRow(saveY,saveColor,saveColor);
		}

}

static void clearGrid()//only called at initialization
{
	for(u8 clearY=maxY;clearY>0;clearY--)
	{
		for(u8 clearX=1;clearX<maxX+1;clearX++)
		{
			p1.board[clearX][clearY]=0;
		}
	}
}

static void checkMatchRow(u8 whichRow, u8 color1, u8 color2)
{
	u8 matchesHori=0,matchStartX=0,destroyFlag=0,incVar=0,checkXinc;

	if(color1!=0)
	{
		for(checkXinc=1;checkXinc<maxX+1;checkXinc++)
			{
				if(p1.board[checkXinc][whichRow]==color1)
					{
						matchesHori++;
						if(matchStartX==0)matchStartX=checkXinc;
						if(matchesHori==3){destroyFlag=1;break;}
					}
				else 
					{
						matchesHori=0;
						matchStartX=0;
					}
			}
	}

	if(destroyFlag==1)
	{
		timerEventsIndex++;
		timerEvents[timerEventsIndex-1]=timer;

		do
		{
			p1.destroyIndex++;
			p1.destroyX[p1.destroyIndex]=matchStartX+incVar;
			p1.destroyY[p1.destroyIndex]=whichRow;
			incVar++;
		}
		while(p1.board[matchStartX+incVar][whichRow]==color1 && matchStartX<maxX);
	}

	destroyFlag=0;
	matchStartX=0;

	if(color2!=0 && color2!=color1)
	{
		for(checkXinc=1;checkXinc<maxX+1;checkXinc++)
			{
				if(p1.board[checkXinc][whichRow]==color2)
					{
						matchesHori++;
						if(matchStartX==0)matchStartX=checkXinc;
						if(matchesHori==3){destroyFlag=1;break;}
					}
				else 
					{
						matchesHori=0;
						matchStartX=0;
					}
			}
	}

	if(destroyFlag==1)//color 2 - doesn't seem to work when we do 2 horiz at once
	{
		timerEventsIndex++;
		timerEvents[timerEventsIndex-1]=timer;

		do
		{
			p1.destroyIndex++;
			p1.destroyX[p1.destroyIndex]=matchStartX+incVar;
			p1.destroyY[p1.destroyIndex]=whichRow;
			incVar++;
		}
		while(p1.board[matchStartX+incVar][whichRow]==color2 && matchStartX<maxX);
	}

	//if(p1.destroyIndex>=3)destroyMatchedTiles();
}

static void checkMatchColumn(u8 whichColumn, u8 color)//this is pulling colors from Y=13
{
	u8 matchesVert=0,matchStartY=0,destroyFlag=0,incVar=0,checkYinc;
	//p1.destroyIndex=0;

	if(color!=0)
	{
		for(checkYinc=1;checkYinc<maxY+1;checkYinc++)
			{
				if(p1.board[whichColumn][checkYinc]==color)
					{
						matchesVert++;
						if(matchStartY==0)matchStartY=checkYinc;
						if(matchesVert==3){destroyFlag=1;break;}
					}
				else 
					{
						matchesVert=0;
						matchStartY=0;
					}
			}
	}

	if(destroyFlag==1)
	{
/*
		VDP_clearText(14,7,10);
		sprintf(debug_string,"vertY=%d",matchStartY);
		VDP_drawText(debug_string,16,7);
*/	
		timerEventsIndex++;
		timerEvents[timerEventsIndex-1]=timer;

		do
		{
			p1.destroyIndex++;
			p1.destroyX[p1.destroyIndex]=whichColumn;
			p1.destroyY[p1.destroyIndex]=matchStartY+incVar;
			incVar++;

			//sprintf(debug_string,"dstry:%d",p1.destroyIndex);
			//VDP_drawText(debug_string,16,8);

		}
		while(p1.board[whichColumn][matchStartY+incVar]==color && matchStartY<maxY+1);
		

	}
//if(p1.destroyIndex>=3)destroyMatchedTiles();
}

static void destroyMatchedTiles()
{
	u8 destroyIndexCopy=p1.destroyIndex;

	do
	{
		p1.board[p1.destroyX[destroyIndexCopy]][p1.destroyY[destroyIndexCopy]]=0;
		destroyIndexCopy--;
	}while(destroyIndexCopy>0);
	
	//doGravity(0);
}

//static void queuedBackgroundUpdate(u8 updateRowBegin, u8 updateRowEnd)
static void queuedBackgroundUpdate()
{

	//if(flag_emptySwitch==1){VDP_clearTileMapRect(BG_A,p1.xpos+p1.xpos,p1.ypos+p1.ypos,4,2);flag_emptySwitch=0;}//clear the tiles if an empty swap was done

	if(p1.destroyIndex>=3)
	{
		VDP_clearTextLineBG(BG_A,16);VDP_clearTextLineBG(BG_A,17);
		VDP_clearTextLineBG(BG_A,18);VDP_clearTextLineBG(BG_A,19);
		VDP_clearTextLineBG(BG_A,20);VDP_clearTextLineBG(BG_A,21);
		//clear 4,5,6,7 connection debug info - doesn't always get overwritten

		do//clear the destroyed tiles
		{
			VDP_clearText(14,13+p1.destroyIndex,20);
			sprintf(debug_string,"%d@%d,%d",p1.destroyIndex,p1.destroyX[p1.destroyIndex],p1.destroyY[p1.destroyIndex]);
			VDP_drawText(debug_string,16,13+p1.destroyIndex);//this will show the last tile which was set to be destroyed

			//insert the clearing of tiles next line:
			VDP_clearTileMapRect(BG_A,p1.destroyX[p1.destroyIndex]+p1.destroyX[p1.destroyIndex],p1.destroyY[p1.destroyIndex]+p1.destroyY[p1.destroyIndex],2,2);
			p1.destroyIndex--;
		}while(p1.destroyIndex>0);		
	}

	if(flag_gravityErase!=0)
	{
		VDP_clearTileMapRect(BG_A,2,2,12,24);//clears the entire P1 board
		flag_gravityErase=0;
	}

	updateBackground();//fully redraw the board tiles
	
}

static void renderScene()
{
	if(p1.flag_redrawAll!=0)
	{
		VDP_clearTileMapRect(BG_A,2,2,12,24);//clears the entire P1 board
		p1.flag_redrawAll=0;
		//flag_emptySwitch=0;
		updateBackground();
	}

	//if(flag_emptySwitch==1){VDP_clearTileMapRect(BG_A,p1.xpos+p1.xpos,p1.ypos+p1.ypos,4,2);flag_emptySwitch=0;}//clear the tiles if an empty swap was done
}

static void processDestroy()
{
	VDP_clearText(14,13,20);
	sprintf(debug_string,"%d@%d,%d",p1.destroyIndex,p1.destroyX[p1.destroyIndex],p1.destroyY[p1.destroyIndex]);
	VDP_drawText(debug_string,16,13);

	do
	{//enum tile{Red=1, Purple=2, Yellow=3, Green=4, Blue=5, Darkblue=6};
		//p1.board[p1.destroyX[p1.destroyIndex]][p1.destroyY[p1.destroyIndex]]=255;//this sets the piece to be destroyed - not what we want
		p1.board[p1.destroyX[p1.destroyIndex]][p1.destroyY[p1.destroyIndex]]+=6;
		p1.destroyIndex--;
		p1.flag_redrawAll=1;
	}while(p1.destroyIndex>0);
}

static void checkTimer()
{
if((timer-timerEvents[0])==destructionTime)
	{
		for(u8 i;i<6;i++)
		{
			p1.board[p1.destroyX[i]][p1.destroyY[i]]=0;
		}

		timerEventsIndex=0;
		doGravity(0);
		p1.flag_redrawAll=1;
	}
}