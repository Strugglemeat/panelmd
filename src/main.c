#include <genesis.h>
#include <resources.h>
#include "functions.h"

int main()
{
	initialize();

	while(1)
	{
		handleInputs(JOY_readJoypad(JOY_1), &p1);
		handleInputs(JOY_readJoypad(JOY_2), &p2);

		timer++;

		animateCursorTimer++;

		if(p1.redraw_delay>0)p1.redraw_delay--;

		doGravity();

		if(p1.destroyIndex!=0)connectedTilesChangeGraphic();

		for(u8 timerIterate=0;timerIterate<MAX_NUMBER_TIMERS;timerIterate++)if(destroyTimer[timerIterate]!=0)destroyTiles();

		SYS_doVBlankProcess();

		animateCursor();

		if(scrolledAmount>0)scrollUp();

		if(p1.flag_redraw!=0 || p2.flag_redraw!=0)renderScene();

		SPR_update();

		//print_debug();
	}

	return 0;
}

static void doGravity()//the parameter here should be the lowest (highest number) row of where the tile(s) landed
{//tiles should not be allowed to fall if they are dissolving! board[x]][y]+=6;//changes the graphic
	u8 gravityDoneFlag=0;
	u8 hasGravity=0;

	u8 finalGravX,finalGravY=0;

	do
	{

	hasGravity=0;
		
		for(u8 gravY=maxY-1;gravY>0;gravY--)
		{
			for(u8 gravX=1;gravX<maxX+1;gravX++)
			{
				//if(board[gravX][gravY]>0 && board[gravX][gravY+1]==0)
				if(board[gravX][gravY]>0 && board[gravX][gravY]<7 && board[gravX][gravY+1]==0)//so that dissolving tiles can't fall
				{
					board[gravX][gravY+1]=board[gravX][gravY];
					board[gravX][gravY]=0;
					gravityDoneFlag=1;
					hasGravity=1;

					finalGravX=gravX;
					if(gravY+1>finalGravY)finalGravY=gravY+1;//we want it to be as high (as far down) as possible

				}
			}
		}
	}while(hasGravity!=0);

	if(gravityDoneFlag==1)
		{
			p1.flag_redraw=1;

			//if(finalGravX>3)checkMatchColumn(finalGravX-2,board[finalGravX][finalGravY]);
			//if(finalGravX>2)checkMatchColumn(finalGravX-1,board[finalGravX][finalGravY]);
			checkMatchColumn(finalGravX,board[finalGravX][finalGravY]);
			//if(finalGravX<maxX)checkMatchColumn(finalGravX+1,board[finalGravX][finalGravY]);
			//if(finalGravX<maxX-1)checkMatchColumn(finalGravX+2,board[finalGravX][finalGravY]);
			//above needs to be refined - causing crashes

			checkMatchRow(finalGravY,board[finalGravX][finalGravY]);
			checkMatchRow(finalGravY-1,board[finalGravX][finalGravY-1]);
			checkMatchRow(finalGravY-2,board[finalGravX][finalGravY-2]);//needs if statements to make sure it doesn't go oob on array(crash)

			//sprintf(debug_string,"finalGravY:%d,color:%d",finalGravY,board[finalGravX][finalGravY]);
			//VDP_drawText(debug_string,2,1);
		}
}

static void checkMatchColumn(u8 whichColumn, u8 color)//this should not be pulling colors from Y=13
{
	u8 matchesVert=0,matchStartY=0,destroyFlag=0,incVar=0,checkYinc;

	if(color!=0)
	{
		for(checkYinc=1;checkYinc<maxY+1;checkYinc++)
			{
				if(board[whichColumn][checkYinc]==color)
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
		do
		{
			p1.destroyIndex++;
			p1.destroyX[p1.destroyIndex]=whichColumn;
			p1.destroyY[p1.destroyIndex]=matchStartY+incVar;
			incVar++;
		}
		while(board[whichColumn][matchStartY+incVar]==color && matchStartY<maxY+1);
	}
}

static void destroyTiles()
{
	u8 timerToUse=0;//the time to destroy is inside each of the members of the array
	while(destroyTimer[timerToUse]==0)timerToUse++;

	if(timer==destroyTimer[timerToUse])
	{
		for (u8 incN=1;incN<MaxPossibleCombo;incN++)
		{
			board[toDestroyX[timerToUse][incN]][toDestroyY[timerToUse][incN]]=0;
			toDestroyX[timerToUse][incN]=0;
			toDestroyY[timerToUse][incN]=0;
//lets add debug here to compare the colors of the tiles that were destroyed
//specifically lets make note if during the connection they are different colors (bug)	

			//if(toDestroyX[timerToUse][incN+1]==0)break;
		}
		destroyTimer[timerToUse]=0;
		p1.flag_redraw=1;
		gravity_delay=GRAVITY_DELAY_AMOUNT;
		//doGravity(0);
	}
}

static void scrollUp()
{
	if(scrolledAmount<blocksize)
	{
		VDP_setVerticalScroll(BG_A,scrollOffset += 1);
		//VDP_setVerticalScrollVSync(BG_A,scrollOffset += 1);
		if(scrollOffset >= 255) scrollOffset = 0;
		scrolledAmount++;

		//p1.cursorY-=1;
		//SPR_setPosition(p1.cursor,p1.cursorX,p1.cursorY);

	}
	else if(scrolledAmount==blocksize)
	{
		pushupRows();
		scrolledAmount=0;
		generateNewRow();
		VDP_setVerticalScroll(BG_A,scrollOffset -= blocksize-1);
	}
}

static void print_debug()
{
	#define debug_print_ypos 0

	//sprintf(debug_string,"FPS:%ld", SYS_getFPS());
	//VDP_drawText(debug_string,34,debug_print_ypos);


	if(SYS_getFPS()<60)
	{
		sprintf(debug_string,"FPS:%ld", SYS_getFPS());
		VDP_drawText(debug_string,34,debug_print_ypos);
	}
	else VDP_clearText(34,debug_print_ypos,6);


	if(p1.xpos<10)VDP_clearText(6,debug_print_ypos,1);
	sprintf(debug_string,"Xpos:%d",p1.xpos);
	VDP_drawText(debug_string,0,debug_print_ypos);

	if(p1.ypos<10)VDP_clearText(14,debug_print_ypos,1);
	sprintf(debug_string,"Ypos:%d",p1.ypos);
	VDP_drawText(debug_string,8,debug_print_ypos);

	VDP_clearText(25,debug_print_ypos,1);
	sprintf(debug_string,"color:%d,%d",board[p1.xpos][p1.ypos],board[p1.xpos+1][p1.ypos]);
	VDP_drawText(debug_string,16,debug_print_ypos);

}

static void checkMatchRow(u8 row, u8 color)
{
	u8 matchesHori=0, destroyFlag=0, beginXdestroy;

	if(color!=0)
	{
		for(u8 i=1;i<maxX;i++)
		{
			if(board[i][row]==color)
			{
				if(matchesHori==0)beginXdestroy=i;
				matchesHori++;
			}
			else if(board[i][row]!=color)matchesHori=0;

			if(matchesHori==3)
			{
				destroyFlag=1;
				break;
			}
		}
	}

	if(destroyFlag==1)
	{
		u8 incVar=0;
		do
		{
			p1.destroyIndex++;
			p1.destroyX[p1.destroyIndex]=beginXdestroy+incVar;
			p1.destroyY[p1.destroyIndex]=row;
			incVar++;
		}
		while(board[beginXdestroy+incVar][row]==color && incVar<maxX);
	}

}