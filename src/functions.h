static void drawBorder();
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
static void checkMatchColumn(u8 whichColumn, u8 color);
static void renderScene();
static void connectedTilesChangeGraphic();
static void destroyTiles();
static void checkGeneratedNewRow();
static void scrollUp();
static void animateCursor();
//static void checkMatchRow(u8 column, u8 row, u8 color1, u8 color2);
static void checkMatchRow(u8 row, u8 color);

#define borderIndex 25
#define tileIndex 1

#define maxX 18//6 for regular, 18 max
#define maxY 12
#define blocksize 16

#define moveDelayAmt 8
#define MAX_SPEEDUP 6//this can't be more than moveDelayAmt!
#define accelerationPace 4//lower is faster
#define accelerationAmount 2//higher is faster
#define raiseDelayAmount 12//lower is faster

#define MaxPossibleCombo 16

#define REDRAW_DELAY_AMOUNT 8

u8 animateCursorTimer;
#define CURSOR_ANIMATE_SPEED 20

enum tile{Red=1, Purple=2, Yellow=3, Green=4, Blue=5, Darkblue=6};

char debug_string[40] = "";

u8 numColors=6;//if 6 it includes the dark blue tile

u8 board[maxX+2][maxY+2];//6x12 playfield, with room around the sides. start at [1][1] for bottom left corner

u8 timer=0;//global, not needed for both players

typedef struct {
Sprite* cursor;
u16 cursorX,cursorY;//changed from u8 to u16 to accommodate larger playfield
u8 xpos,ypos;

u8 moveDelay,acceleration;
u16 lastDirInput;
u8 lastDirInputHowMany;
u8 raiseDelay;

u8 destroyIndex;
u8 destroyX[MaxPossibleCombo+1];
u8 destroyY[MaxPossibleCombo+1];//max 16 destroyed in one move - 0 of index isn't used

u8 hasSwitched;

u8 flag_redraw;

u8 redraw_delay;
Sprite* switch1;
Sprite* switch2;
u16 switch1x,switch2x;//255 will overflow - 320 wide screen
u8 switchy;
} Player;

Player p1;
Player p2;

u8 gravity_delay;
#define GRAVITY_DELAY_AMOUNT 16

//u8 whichTimer=0;
#define MAX_NUMBER_TIMERS 8
u8 destroyTimer[MAX_NUMBER_TIMERS];//# timers (max 2 per player conns at any one time),16 tiles per conn, X and Y
u8 toDestroyX[4][17];
u8 toDestroyY[4][17];

#define destroyDelay 320

u8 scrollOffset=maxY-blocksize+2;
u8 scrolledAmount=0;

u8 scrollUpDelay=0;

static void handleInputs(u16 buttons, Player* player);//function definition has to be after struct definition

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
	for(iX=2;iX<38;iX+=2)//top and bottom rows
	{
		VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, FALSE, FALSE, borderIndex), iX, 0, 2, 2);
		VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, FALSE, FALSE, borderIndex+12), iX, 26, 2, 2);
	}

	for(iY=2;iY<25;iY+=2)//left and right sides, inner walls
	{
		VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, FALSE, FALSE, borderIndex+4), 0, iY, 2, 2);
		VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, FALSE, FALSE, borderIndex+4), 38, iY, 2, 2);
	}

	VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, FALSE, FALSE, borderIndex+8), 0, 0, 2, 2);

	VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, FALSE, TRUE, borderIndex+9), 38, 0, 1, 1);
	VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, FALSE, TRUE, borderIndex+8), 39, 0, 1, 1);
	VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, FALSE, TRUE, borderIndex+11), 38, 1, 1, 1);
	VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, FALSE, TRUE, borderIndex+10), 39, 1, 1, 1);

	VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, TRUE, TRUE, borderIndex+8), 39, 27, 1, 1);
	VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, TRUE, TRUE, borderIndex+9), 38, 27, 1, 1);
	VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, TRUE, TRUE, borderIndex+10), 39, 26, 1, 1);
	VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, TRUE, TRUE, borderIndex+11), 38, 26, 1, 1);

	VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, TRUE, FALSE, borderIndex+8), 0, 27, 1, 1);
	VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, TRUE, FALSE, borderIndex+9), 1, 27, 1, 1);
	VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, TRUE, FALSE, borderIndex+10), 0, 26, 1, 1);
	VDP_fillTileMapRectInc(BG_B, TILE_ATTR_FULL(PAL2, 1, TRUE, FALSE, borderIndex+11), 1, 26, 1, 1);
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
	VDP_setPalette(PAL2, bgtilevert.palette->data);
	VDP_loadTileSet(bgtilevert.tileset,borderIndex,CPU);
	VDP_loadTileSet(bgtilehori.tileset,borderIndex+4,CPU);
	VDP_loadTileSet(bgtilecorner.tileset,borderIndex+8,CPU);
	VDP_loadTileSet(bgtilebottom.tileset,borderIndex+12,CPU);
	drawBorder();

	//tiles
	VDP_setPalette(PAL3, alltiles.palette->data);
	VDP_loadTileSet(alltiles.tileset,tileIndex,DMA);

	clearGrid();

	insertInitialRowData();
	generateNewRow();
	updateBackground();

	SYS_enableInts();

	VDP_setHilightShadow(1);

	p1.xpos=1;
	p1.ypos=maxY;

	p2.xpos=maxX-1;
	p2.ypos=1;

	SPR_init();

	//******BEGIN P1 SPRITE STUFF********
	p1.cursor = SPR_addSprite(&cursor,0,0,TILE_ATTR(PAL1,0,FALSE,FALSE));
	SPR_setVisibility(p1.cursor,HIDDEN);//make it hidden while doing loading/init stuff
	p1.cursorX=16+((p1.xpos-1)*blocksize)-4;
	p1.cursorY=16+((p1.ypos-1)*blocksize)-2;
	SPR_setVisibility(p1.cursor,VISIBLE);
	SPR_setPriorityAttribut(p1.cursor, TRUE);//because of hilight/shadow
	SPR_setPosition(p1.cursor,p1.cursorX,p1.cursorY);

	p1.switch1 = SPR_addSprite(&sprite_tiles,0,0,TILE_ATTR(PAL3,0,FALSE,FALSE));
	SPR_setVisibility(p1.switch1,HIDDEN);
	p1.switch2 = SPR_addSprite(&sprite_tiles,0,0,TILE_ATTR(PAL3,0,FALSE,FALSE));
	SPR_setVisibility(p1.switch2,HIDDEN);

	SPR_setPriorityAttribut(p1.switch1, TRUE);
	SPR_setPriorityAttribut(p1.switch2, TRUE);

	//******BEGIN P2 SPRITE STUFF********
	p2.cursor = SPR_addSprite(&cursor2,0,0,TILE_ATTR(PAL1,0,FALSE,FALSE));
	SPR_setVisibility(p2.cursor,HIDDEN);//make it hidden while doing loading/init stuff
	p2.cursorX=16+((p2.xpos-1)*blocksize)-4;
	p2.cursorY=16+((p2.ypos-1)*blocksize)-2;
	SPR_setVisibility(p2.cursor,VISIBLE);
	SPR_setPriorityAttribut(p2.cursor, TRUE);//because of hilight/shadow
	SPR_setPosition(p2.cursor,p2.cursorX,p2.cursorY);

	p2.switch1 = SPR_addSprite(&sprite_tiles,0,0,TILE_ATTR(PAL3,0,FALSE,FALSE));
	SPR_setVisibility(p2.switch1,HIDDEN);
	p2.switch2 = SPR_addSprite(&sprite_tiles,0,0,TILE_ATTR(PAL3,0,FALSE,FALSE));
	SPR_setVisibility(p2.switch2,HIDDEN);

	SPR_setPriorityAttribut(p2.switch1, TRUE);
	SPR_setPriorityAttribut(p2.switch2, TRUE);

	//SPR_update();
}

static void drawTile(u8 x, u8 y, u8 color)
{
	u8 PRIORITY=1;

	if(color>numColors)//shadow the tiles which are being destroyed
		{
			color-=6;
			PRIORITY=0;
		}
	if(y>maxY)//shadow the new tiles which are coming up from the bottom
		{
			PRIORITY=0;
		}

	switch(color)
	{//enum tile{Red=1, Purple=2, Yellow=3, Green=4, Blue=5, Darkblue=6};
		case Red:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+color-1), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+1+color-1), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+12+color-1), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+13+color-1), x+x+1, y+y+1, 1, 1);
		break;

		case Purple:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+color), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+1+color), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+12+color), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+13+color), x+x+1, y+y+1, 1, 1);
		break;

		case Yellow:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+color+1), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+1+color+1), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+12+color+1), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+13+color+1), x+x+1, y+y+1, 1, 1);
		break;

		case Green:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+color+2), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+1+color+2), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+12+color+2), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+13+color+2), x+x+1, y+y+1, 1, 1);
		break;

		case Blue:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+color+3), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+1+color+3), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+12+color+3), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+13+color+3), x+x+1, y+y+1, 1, 1);
		break;

		case Darkblue:
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+color+4), x+x, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+1+color+4), x+x+1, y+y, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+12+color+4), x+x, y+y+1, 1, 1);
		VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, PRIORITY, FALSE, FALSE, tileIndex+13+color+4), x+x+1, y+y+1, 1, 1);
		break;
	}
}

//static void updateBackground(u8 yStart, u8 yEnd)
static void updateBackground()
{
	u8 iX,iY;

	u8 yStart=1;//1 is the topmost row
	u8 yEnd=maxY+2;//maxY+1 is the bottom row

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
	else if(checkVertFlag==0)p1.flag_redraw=1;
	//else if(checkVertFlag==0)pushupRows();
}

static void renderScene()//needs to be refactored for both players
{
	if(p1.flag_redraw==1 || p2.flag_redraw==2)//redraw the entire scene
	{
		VDP_clearTileMapRect(BG_A,2,2,maxX+maxX,maxY+maxY);//clears the entire P1 board
		//VDP_clearTileMapRect(BG_A,p1.cursorX,p1.cursorY,p1.cursorX+2,p1.cursorY+2);
		p1.flag_redraw=0;
		p2.flag_redraw=0;
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
	
	if(p1.flag_redraw==3 && p1.redraw_delay>0)//making this an else if was causing a crash
	{
		p1.switch1x+=2;
		p1.switch2x-=2;
		SPR_setPosition(p1.switch1,p1.switch1x,p1.switchy);
		SPR_setPosition(p1.switch2,p1.switch2x,p1.switchy);
		//SPR_update();
	}
	else if(p1.flag_redraw==3 && p1.redraw_delay==0)
	{
		SPR_setVisibility(p1.switch1,HIDDEN);
		SPR_setVisibility(p1.switch2,HIDDEN);
		//SPR_update();

		p1.flag_redraw=0;
		updateBackground();
	}
}

static void animateCursor()
{
	if (animateCursorTimer==CURSOR_ANIMATE_SPEED)
		{
			SPR_setFrame(p1.cursor, 1);
			//SPR_update();
		}
	if (animateCursorTimer==CURSOR_ANIMATE_SPEED+CURSOR_ANIMATE_SPEED)
		{
			SPR_setFrame(p1.cursor, 0);
			//SPR_update();
			animateCursorTimer=0;
		}
}

static void handleInputs(u16 buttons, Player* player)
{
	if(buttons==player->lastDirInput && buttons!=0)player->lastDirInputHowMany++;//count for acceleration
	else if(buttons!=player->lastDirInput){player->lastDirInputHowMany=0;player->acceleration=0;}//if changing directions
	else if(buttons==0){player->lastDirInput=0;player->lastDirInputHowMany=0;player->acceleration=0;}//if no more dpad input

	if(player->lastDirInputHowMany%accelerationPace==0 && player->lastDirInputHowMany>accelerationPace)player->acceleration+=accelerationAmount;
	if(player->acceleration>MAX_SPEEDUP)player->acceleration=MAX_SPEEDUP;

	if(player->moveDelay>0)player->moveDelay--;
	else if(player->moveDelay==0)
	{

		if ((buttons & BUTTON_UP) && player->ypos > 1)
		{
			player->ypos--;
		    player->cursorY-=blocksize;

		    player->moveDelay=moveDelayAmt-player->acceleration;
		    player->lastDirInput=buttons;
		    SPR_setPosition(player->cursor,player->cursorX,player->cursorY);
			//SPR_update();
		}
		if ((buttons & BUTTON_DOWN) && player->ypos < maxY)
		    {
		    	player->ypos++;
		    	player->cursorY+=blocksize;

		    	player->moveDelay=moveDelayAmt-player->acceleration;
		    	player->lastDirInput=buttons;
			    SPR_setPosition(player->cursor,player->cursorX,player->cursorY);
				//SPR_update();
		    }
		if ((buttons & BUTTON_LEFT) && player->xpos > 1)
		    {
		    	player->xpos--;
		    	player->cursorX-=blocksize;

		    	player->moveDelay=moveDelayAmt-player->acceleration;
		    	player->lastDirInput=buttons;
			    SPR_setPosition(player->cursor,player->cursorX,player->cursorY);
				//SPR_update();
		    }
	    if ((buttons & BUTTON_RIGHT) && player->xpos < maxX-1)
		    {
		    	player->xpos++;
		    	player->cursorX+=blocksize;

		    	player->moveDelay=moveDelayAmt-player->acceleration;
		    	player->lastDirInput=buttons;
			    SPR_setPosition(player->cursor,player->cursorX,player->cursorY);
				//SPR_update();
		    }
	}

	if(((buttons & BUTTON_A) || buttons & BUTTON_C) && player->hasSwitched==0 && board[player->xpos][player->ypos]<=numColors && board[player->xpos+1][player->ypos]<=numColors)
		{
			u8 color1=0,color2=0;
	        color1=board[player->xpos][player->ypos];
	        color2=board[player->xpos+1][player->ypos];//this is to see if we swapped with an empty, to do gravity

	        if((color1==0 && board[player->xpos+1][player->ypos+1]==0) || (color2==0 && board[player->xpos][player->ypos+1]==0))
		        {//empty swap with nothing below
			        SPR_setVisibility(player->switch1,HIDDEN);
					SPR_setVisibility(player->switch2,HIDDEN);
					//SPR_update();
			        return;
		        }

			if(color1<=numColors && color2<=numColors)//effect the swap
				{
					board[player->xpos][player->ypos]=board[player->xpos+1][player->ypos];
					board[player->xpos+1][player->ypos]=color1;

					checkMatchColumn(player->xpos,color2);
					checkMatchColumn(player->xpos+1,color1);

					//checkMatchRow(u8 column, u8 row, u8 color1, u8 color2)
					//checkMatchRow(player->xpos,player->ypos,color1,color2);
					checkMatchRow(player->ypos,color1);
					checkMatchRow(player->ypos,color2);

					//player->flag_redraw=2;
				}
			//else if(color1>numColors || color2>numColors)return;//if one of the tiles in the cursor is disappearing, we can't use it

			if(color1==0 || color2==0)//empty swap
				{
					gravity_delay=GRAVITY_DELAY_AMOUNT;
				}

			player->hasSwitched=1;
			player->flag_redraw=2;
		}
	else if((!(buttons & BUTTON_A)) && (!(buttons & BUTTON_C))){player->hasSwitched=0;}

	if((!(buttons & BUTTON_A)) && (!(buttons & BUTTON_C))){player->hasSwitched=0;}

	if(player->raiseDelay>0)player->raiseDelay--;
	else if ((buttons & BUTTON_B) && checkTopRow()==0 && player->raiseDelay==0)
	{
		if(scrolledAmount==0)scrolledAmount=1;
		player->raiseDelay=raiseDelayAmount;
	}
	
	if ((buttons & BUTTON_B) && player->raiseDelay!=0)
	{
		player->raiseDelay=0;
	}

	if (buttons & BUTTON_START) 
	{
		player->flag_redraw=1;
	}
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

//we also need to pushup inside each of the destruction arrays
//board[x][y]>6
//toDestroyY[whichTimer][p1.destroyIndex]=p1.destroyY[p1.destroyIndex];

	for(u8 i=0;i<MAX_NUMBER_TIMERS;i++)
	{
		if(destroyTimer[i]!=0)
		{
			for(u8 y=0;y<MaxPossibleCombo;y++)
			{
				toDestroyY[i][y]=toDestroyY[i][y]-1;
			}
		}
	}

	p1.flag_redraw=1;
}

static void connectedTilesChangeGraphic()
{
	//sprintf(debug_string,"%d@%d,%d",p1.destroyIndex,p1.destroyX[p1.destroyIndex],p1.destroyY[p1.destroyIndex]);
	//VDP_drawText(debug_string,16,13);

	u8 debugDestroyCount=0;

	//whichTimer++;
	//if(whichTimer>MAX_NUMBER_TIMERS)whichTimer=0;
	u8 whichTimer=0;
	while(destroyTimer[whichTimer]!=0)whichTimer++;	//find the first open timer
	
	do
	{
		if(board[p1.destroyX[p1.destroyIndex]][p1.destroyY[p1.destroyIndex]]<=numColors)
		{//otherwise, it'll +6 the same tile twice if it's involved in both a hori and vert match
			board[p1.destroyX[p1.destroyIndex]][p1.destroyY[p1.destroyIndex]]+=6;//changes the graphic
			toDestroyX[whichTimer][p1.destroyIndex]=p1.destroyX[p1.destroyIndex];
			toDestroyY[whichTimer][p1.destroyIndex]=p1.destroyY[p1.destroyIndex];
			debugDestroyCount++;
		}
		p1.destroyIndex--;
	}while(p1.destroyIndex>0);

	destroyTimer[whichTimer]=timer+destroyDelay;
	if(destroyTimer[whichTimer]==0)destroyTimer[whichTimer]++;//dont let it be zero  - WHAT IF there's a timer in use at 1 already?

	//sprintf(debug_string,"combo:%d",debugDestroyCount);
	//VDP_drawText(debug_string,26,0);
}