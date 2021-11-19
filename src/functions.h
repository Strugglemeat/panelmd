static void drawBorder();
static void handleInput();
static void print_debug();
static u8 randomRange(u8 rangeStart, u8 rangeEnd);
static void updateBackground();
static void drawTile(u8 x, u8 y, u8 color);
static void insertInitialRowData();
static void generateNewRow();
static void pushupRows();
static u8 checkTopRow();
static void doGravity();
static void clearGrid();
static void checkMatchRow(u8 whichRow, u8 color);
static void checkMatchColumn(u8 whichColumn, u8 color);
static void renderScene();
static void connectedTilesChangeGraphic();
static void destroyTiles();
static void checkGeneratedNewRow();

#define borderIndex 1
#define tileIndex 5

#define maxX 18//6 for regular, 18 max
#define maxY 12
#define blocksize 16
#define p1boardstartX 16
#define p2boardstartX 208
#define boardstartY 16

#define moveDelayAmt 8
#define MAX_SPEEDUP 6//this can't be more than moveDelayAmt!
#define accelerationPace 4//lower is faster
#define accelerationAmount 2//higher is faster
#define raiseDelayAmount 12//lower is faster

#define MaxInOneMove 16

#define REDRAW_DELAY_AMOUNT 8

enum tile{Red=1, Purple=2, Yellow=3, Green=4, Blue=5, Darkblue=6};

char debug_string[40] = "";

u8 numColors=6;//if 6 it includes the dark blue tile

typedef struct {
Sprite* cursor;
u16 cursorX,cursorY;//changed from u8 to u16 to accommodate larger playfield
u8 xpos,ypos;

u8 moveDelay,acceleration;
u16 lastDirInput;
u8 lastDirInputHowMany;
u8 raiseDelay;

u8 destroyIndex;
u8 destroyX[MaxInOneMove+1];
u8 destroyY[MaxInOneMove+1];//max 16 destroyed in one move - 0 of index isn't used

u8 hasSwitched;

u8 flag_redraw;

u8 redraw_delay;
Sprite* switch1;
Sprite* switch2;
u16 switch1x,switch2x;//255 will overflow - 320 wide screen
u8 switchy;
} Player;

Player p1;

u8 board[maxX+2][maxY+2];//6x12 playfield, with room around the sides. start at [1][1] for bottom left corner
u8 timer=0;//global, not needed for both players

static void clearGrid()//only called at initialization
{
	VDP_clearTileMapRect(BG_A,2,2,maxX+maxX,maxY+maxY);
	for(u8 clearY=maxY;clearY>0;clearY--)
	{
		for(u8 clearX=1;clearX<maxX+1;clearX++)
		{
			board[clearX][clearY]=0;
		}
	}
}

static void drawBorder()//only called at initialization
{
	u8 iX,iY;
	for(iX=0;iX<40;iX+=2)//top and bottom rows
	{
		VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, FALSE, FALSE, borderIndex), iX, 0, 2, 2);
		VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, FALSE, FALSE, borderIndex), iX, 26, 2, 2);
	}

	for(iY=2;iY<25;iY+=2)//left and right sides, inner walls
	{
		VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, FALSE, FALSE, borderIndex), 0, iY, 2, 2);
		VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, FALSE, FALSE, borderIndex), 38, iY, 2, 2);
	}
}

static void insertInitialRowData()//only called at initialization
{
	board[6][maxY-2]=randomRange(1,numColors);
	board[6][maxY-1]=randomRange(1,numColors);
	board[2][maxY]=randomRange(1,numColors);
	board[3][maxY]=randomRange(1,numColors);
	board[4][maxY]=randomRange(1,numColors);
	board[5][maxY]=randomRange(1,numColors);
	board[6][maxY]=randomRange(1,numColors);
}

static void initialize()
{
SYS_disableInts();

VDP_setScreenWidth320();
VDP_setScreenHeight224();
VDP_loadFontData(tileset_Font_Namco.tiles, 96, CPU);
VDP_setPalette(PAL1, cursor.palette->data);
VDP_setTextPlane(BG_B);
VDP_setScrollingMode(HSCROLL_PLANE , VSCROLL_PLANE);

//border
VDP_setPalette(PAL2, bgtile.palette->data);
VDP_loadTileSet(bgtile.tileset,borderIndex,DMA);
drawBorder();

//tiles
VDP_setPalette(PAL3, alltiles.palette->data);
VDP_loadTileSet(alltiles.tileset,tileIndex,DMA);

clearGrid();

insertInitialRowData();
updateBackground();

SYS_enableInts();

SPR_init();
p1.cursor = SPR_addSprite(&cursor,0,0,TILE_ATTR(PAL1,0,FALSE,FALSE));
SPR_setVisibility(p1.cursor,HIDDEN);//make it hidden while doing loading/init stuff

p1.xpos=1;
p1.ypos=maxY;
p1.cursorX=p1boardstartX+((p1.xpos-1)*blocksize)-2;
p1.cursorY=boardstartY+((p1.ypos-1)*blocksize)-2;

VDP_setHilightShadow(1);

SPR_setVisibility(p1.cursor,VISIBLE);
SPR_setPriorityAttribut(p1.cursor, TRUE);//because of hilight/shadow
SPR_setPosition(p1.cursor,p1.cursorX,p1.cursorY);
SPR_update();

p1.switch1 = SPR_addSprite(&sprite_tiles,0,0,TILE_ATTR(PAL3,0,FALSE,FALSE));
SPR_setVisibility(p1.switch1,HIDDEN);
SPR_setPriorityAttribut(p1.switch1, TRUE);//because of hilight/shadow
p1.switch2 = SPR_addSprite(&sprite_tiles,0,0,TILE_ATTR(PAL3,0,FALSE,FALSE));
SPR_setVisibility(p1.switch2,HIDDEN);
SPR_setPriorityAttribut(p1.switch2, TRUE);//because of hilight/shadow

p1.flag_redraw=0;
}

static void drawTile(u8 x, u8 y, u8 color)
{
	switch(color)
	{//enum tile{Red=1, Purple=2, Yellow=3, Green=4, Blue=5, Darkblue=6};
		case Red:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+color-1), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+1+color-1), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+12+color-1), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+13+color-1), x+x+1, y+y+1, 1, 1);
		break;

		case Purple:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+color), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+1+color), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+12+color), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+13+color), x+x+1, y+y+1, 1, 1);
		break;

		case Yellow:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+color+1), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+1+color+1), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+12+color+1), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+13+color+1), x+x+1, y+y+1, 1, 1);
		break;

		case Green:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+color+2), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+1+color+2), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+12+color+2), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+13+color+2), x+x+1, y+y+1, 1, 1);
		break;

		case Blue:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+color+3), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+1+color+3), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+12+color+3), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+13+color+3), x+x+1, y+y+1, 1, 1);
		break;

		case Darkblue:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+color+4), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+1+color+4), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+12+color+4), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 1, FALSE, FALSE, tileIndex+13+color+4), x+x+1, y+y+1, 1, 1);
		break;

		case Red+6:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+color-1-6), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+1+color-1-6), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+12+color-1-6), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+13+color-1-6), x+x+1, y+y+1, 1, 1);
		break;

		case Purple+6:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+color-6), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+1+color-6), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+12+color-6), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+13+color-6), x+x+1, y+y+1, 1, 1);
		break;

		case Yellow+6:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+color+1-6), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+1+color+1-6), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+12+color+1-6), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+13+color+1-6), x+x+1, y+y+1, 1, 1);
		break;

		case Green+6:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+color+2-6), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+1+color+2-6), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+12+color+2-6), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+13+color+2-6), x+x+1, y+y+1, 1, 1);
		break;

		case Blue+6:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+color+3-6), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+1+color+3-6), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+12+color+3-6), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+13+color+3-6), x+x+1, y+y+1, 1, 1);
		break;

		case Darkblue+6:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+color+4-6), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+1+color+4-6), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+12+color+4-6), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndex+13+color+4-6), x+x+1, y+y+1, 1, 1);
		break;

		case 255:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, 0), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, 0), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, 0), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, 0), x+x+1, y+y+1, 1, 1);
		break;
	}
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
			if(board[iX][iY]!=0)
				{
				drawTile(iX,iY,board[iX][iY]);
				}
			}
		}
}

static void renderScene()
{
	if(p1.flag_redraw==1)//redraw the entire scene
	{
		VDP_clearTileMapRect(BG_A,2,2,maxX+maxX,maxY+maxY);//clears the entire P1 board
		p1.flag_redraw=0;
		updateBackground();
	}
	else if(p1.flag_redraw==2)//after a blank swap or regular swap
	{
		p1.switch1x=p1.cursorX+2;
		p1.switchy=p1.cursorY+2;
		p1.switch2x=p1.cursorX+16+2;

		if(board[p1.xpos+1][p1.ypos]<=numColors)SPR_setFrame(p1.switch1, board[p1.xpos+1][p1.ypos]-1);
		if(board[p1.xpos][p1.ypos]<=numColors)SPR_setFrame(p1.switch2, board[p1.xpos][p1.ypos]-1);

		if(board[p1.xpos+1][p1.ypos]>0)SPR_setVisibility(p1.switch1,VISIBLE);

		if(board[p1.xpos][p1.ypos]>0)SPR_setVisibility(p1.switch2,VISIBLE);

		p1.flag_redraw=3;
		p1.redraw_delay=REDRAW_DELAY_AMOUNT;
	}
	else if(p1.redraw_delay==REDRAW_DELAY_AMOUNT-1)
	{//putting it here removes the blinking effect on a swap with a blank tile
		VDP_clearTileMapRect(BG_A,p1.xpos+p1.xpos,p1.ypos+p1.ypos,4,2);
	}
	else if(p1.flag_redraw==3 && p1.redraw_delay>0)
	{
		p1.switch1x+=2;
		p1.switch2x-=2;
		SPR_setPosition(p1.switch1,p1.switch1x,p1.switchy);
		SPR_setPosition(p1.switch2,p1.switch2x,p1.switchy);
		SPR_update();
	}
	else if(p1.flag_redraw==3 && p1.redraw_delay==0)
	{
		SPR_setVisibility(p1.switch1,HIDDEN);
		SPR_setVisibility(p1.switch2,HIDDEN);
		SPR_update();

		p1.flag_redraw=0;
		updateBackground();
	}
}

static u8 checkTopRow()
{
	u8 emptyCheck=0;

		for(u8 xPos=1;xPos<maxX+1;xPos++)
		{
			if(board[xPos][1]>0)emptyCheck=1;
		}

	if(emptyCheck>0)return 1;
	else return 0;
}

static void pushupRows()
{
	for(u8 yPos=1;yPos<maxY+1;yPos++)
	{
		for(u8 xPos=1;xPos<maxX+1;xPos++)
		{
			board[xPos][yPos]=board[xPos][yPos+1];
		}
	}

	p1.flag_redraw=1;
}

static u8 randomRange(u8 rangeStart, u8 rangeEnd)
{
	return (random() % (rangeEnd + 1 - rangeStart)) + rangeStart;
}

static void generateNewRow()
{
	for(u8 newX=1;newX<maxX+1;newX++)
	{
		board[newX][maxY+1]=randomRange(1,numColors);
	}

	checkGeneratedNewRow();//check for column matches
}

static void checkGeneratedNewRow()
{
	for(u8 gencheckXinc=1;gencheckXinc<maxX+1;gencheckXinc++)//check for rows of matching
		{
			if(board[gencheckXinc][maxY+1]==board[gencheckXinc+1][maxY+1])
				{
					if(board[gencheckXinc+1][maxY+1]<6)board[gencheckXinc+1][maxY+1]++;
					else if(board[gencheckXinc+1][maxY+1]==6)board[gencheckXinc+1][maxY+1]=1;
				}
		}

	//and we now need to manage the vertical matches of the newly created rows

	u8 columnColor,columnNumber,checkGenYinc,checkMatchVert=0,checkVertFlag=0;

	for(columnNumber=1;columnNumber<maxX+1;columnNumber++)
	{
		columnColor=board[columnNumber][maxY-1];
		for(checkGenYinc=maxY+1;checkGenYinc>maxY-1;checkGenYinc--)
			{
				if(board[columnNumber][checkGenYinc]==columnColor)
					{
						checkMatchVert++;
						if(checkMatchVert>=2){checkVertFlag=1;break;}
					}
				else 
					{
						checkMatchVert=0;
					}
			}
	}

	if(checkVertFlag==1)generateNewRow();
	else if(checkVertFlag==0)pushupRows();
}

static void print_debug()
{
	sprintf(debug_string,"FPS:%ld", SYS_getFPS());VDP_drawText(debug_string,34,0);
/*
	if(SYS_getFPS()<60)
	{
		sprintf(debug_string,"FPS:%ld", SYS_getFPS());
		VDP_drawText(debug_string,34,1);
	}
	else VDP_clearText(34,1,6);*/

	if(p1.xpos<10)VDP_clearText(6,0,1);
	sprintf(debug_string,"Xpos:%d",p1.xpos);
	VDP_drawText(debug_string,0,0);

	if(p1.ypos<10)VDP_clearText(14,0,1);
	sprintf(debug_string,"Ypos:%d",p1.ypos);
	VDP_drawText(debug_string,8,0);

	VDP_clearText(23,0,1);
	sprintf(debug_string,"color:%d",board[p1.xpos][p1.ypos]);
	VDP_drawText(debug_string,16,0);
}