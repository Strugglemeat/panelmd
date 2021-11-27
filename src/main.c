#include <genesis.h>
#include <resources.h>
#include "functions.h"

u8 destroyTimer[4];//4 timers (max 2 per player conns at any one time),16 tiles per conn, X and Y
u8 toDestroyX[4][17];
u8 toDestroyY[4][17];

#define destroyDelay 32

u8 gravity_delay;
#define GRAVITY_DELAY_AMOUNT 16

u8 scrollOffset;
u8 scrolledAmount=0;

int main()
{
	initialize();
	scrollOffset=p1.ypos-blocksize+2;

	while(1)
	{
		handleInput();
		timer++;
		if(p1.redraw_delay>0)p1.redraw_delay--;
		if(gravity_delay==1)doGravity();
		if(gravity_delay>0)gravity_delay--;
		if(p1.destroyIndex!=0)connectedTilesChangeGraphic();
		if(destroyTimer[0]!=0 || destroyTimer[1]!=0 || destroyTimer[2]!=0 || destroyTimer[3]!=0)destroyTiles();
		SYS_doVBlankProcess();
		if(p1.flag_redraw!=0)renderScene();
		print_debug();
	}

	return 0;
}

static void handleInput()
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
	    if ((value1 & BUTTON_LEFT) && p1.xpos>1)   
		    {
		    	p1.xpos--;
		    	p1.cursorX-=blocksize;

		    	p1.moveDelay=moveDelayAmt-p1.acceleration;
		    	p1.lastDirInput=value1;
			    SPR_setPosition(p1.cursor,p1.cursorX,p1.cursorY);
				SPR_update();
		    }
	    if ((value1 & BUTTON_RIGHT) && p1.xpos < maxX-1)   
		    {
		    	p1.xpos++;
		    	p1.cursorX+=blocksize;

		    	p1.moveDelay=moveDelayAmt-p1.acceleration;
		    	p1.lastDirInput=value1;
			    SPR_setPosition(p1.cursor,p1.cursorX,p1.cursorY);
				SPR_update();
		    }
	    if ((value1 & BUTTON_UP) && p1.ypos>1)      
		    {
			    p1.ypos--;
			    p1.cursorY-=blocksize;

			    p1.moveDelay=moveDelayAmt-p1.acceleration;
			    p1.lastDirInput=value1;
			    SPR_setPosition(p1.cursor,p1.cursorX,p1.cursorY);
				SPR_update();
		    }//1,1 is the bottom left...
	    if ((value1 & BUTTON_DOWN) && p1.ypos<maxY)    
		    {
		    	p1.ypos++;
		    	p1.cursorY+=blocksize;

		    	p1.moveDelay=moveDelayAmt-p1.acceleration;
		    	p1.lastDirInput=value1;
			    SPR_setPosition(p1.cursor,p1.cursorX,p1.cursorY);
				SPR_update();
		    }
   	}

	if(p1.raiseDelay>0)p1.raiseDelay--;
	else if (checkTopRow()==0 && p1.raiseDelay==0 && (value1 & BUTTON_B))
	{
		generateNewRow();
		p1.raiseDelay=raiseDelayAmount;
	}

	if(((value1 & BUTTON_A) || value1 & BUTTON_C) && p1.hasSwitched==0)
	{
		//VDP_clearText(14,13,20);VDP_clearText(14,14,20);//clears the debug text for matches

		p1.hasSwitched=1;

		u8 color1=0,color2=0;
        color1=board[p1.xpos][p1.ypos];
        color2=board[p1.xpos+1][p1.ypos];//this is to see if we swapped with an empty, to do gravity

		if(color1<=numColors && color2<=numColors)
		{
			board[p1.xpos][p1.ypos]=board[p1.xpos+1][p1.ypos];
			board[p1.xpos+1][p1.ypos]=color1;

			p1.flag_redraw=2;
			
			checkMatchRow(p1.ypos,color1);
			checkMatchRow(p1.ypos,color2);
			checkMatchColumn(p1.xpos,color2);
			checkMatchColumn(p1.xpos+1,color1);
		}

		if(color1==0 || color2==0)//empty swap
			{
				//p1.flag_redraw=2;
				gravity_delay=GRAVITY_DELAY_AMOUNT;
			}
	}

	if((!(value1 & BUTTON_A)) && (!(value1 & BUTTON_C))){p1.hasSwitched=0;}

	//if(value1 & BUTTON_START)doGravity(0);//DEBUG
	if(value1 & BUTTON_START)
	{
		if(board[1][maxY+1]==0)generateNewRow();

		VDP_setVerticalScroll(BG_A,scrollOffset += 1);
		//VDP_setVerticalScrollVSync(BG_A,scrollOffset += 1);
		if(scrollOffset >= 255) scrollOffset = 0;
		scrolledAmount++;

		p1.cursorY-=1;
		SPR_setPosition(p1.cursor,p1.cursorX,p1.cursorY);
		SPR_update();
	}
}

static void doGravity()//the parameter here should be the lowest (highest number) row of where the tile(s) landed
{
	u8 gravityDoneFlag=0;
	u8 hasGravity=0;

	u8 dropCheckX,dropCheckY;

	do
	{
	hasGravity=0;
		for(u8 gravY=maxY-1;gravY>0;gravY--)
		{
			for(u8 gravX=1;gravX<maxX+1;gravX++)
			{
				if(board[gravX][gravY]>0 && board[gravX][gravY+1]==0)
				{
					board[gravX][gravY+1]=board[gravX][gravY];
					board[gravX][gravY]=0;
					gravityDoneFlag=1;
					hasGravity++;

					dropCheckX=gravX;
					dropCheckY=gravY+1;

					checkMatchColumn(dropCheckX,board[dropCheckX][dropCheckY]);
					checkMatchRow(dropCheckY,board[dropCheckX][dropCheckY]);
				}
			}
		}
	}while(hasGravity!=0);

	if(gravityDoneFlag>0)
		{
			p1.flag_redraw=1;
			/*
			VDP_clearText(19,0,2);
			sprintf(debug_string,"dcX:%d,dcY:%d,color:%d",dropCheckX,dropCheckY,board[dropCheckX][dropCheckY]);
			VDP_drawText(debug_string,0,0);
			
			sprintf(debug_string,"gravColor:%d",board[dropCheckX][dropCheckY]);
			VDP_drawText(debug_string,16,10);
			*/
		}
}



static void checkMatchRow(u8 whichRow, u8 color)
{
	u8 matchesHori=0,matchStartX=0,destroyFlag=0,incVar=0,checkXinc;

	if(color!=0)
	{
		for(checkXinc=1;checkXinc<maxX+1;checkXinc++)
			{
				if(board[checkXinc][whichRow]==color)
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
		do
		{
			p1.destroyIndex++;
			p1.destroyX[p1.destroyIndex]=matchStartX+incVar;
			p1.destroyY[p1.destroyIndex]=whichRow;
			incVar++;
		}
		while(board[matchStartX+incVar][whichRow]==color && matchStartX<maxX);
	}
}

static void checkMatchColumn(u8 whichColumn, u8 color)//this is pulling colors from Y=13
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

static void connectedTilesChangeGraphic()
{
	//sprintf(debug_string,"%d@%d,%d",p1.destroyIndex,p1.destroyX[p1.destroyIndex],p1.destroyY[p1.destroyIndex]);
	//VDP_drawText(debug_string,16,13);

	u8 debugDestroyCount=0;

	u8 whichTimer=0;
	while(destroyTimer[whichTimer]!=0)whichTimer++;	//find the first open timer (there are 4 total destroyTimer[4])
	
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
	if(destroyTimer[whichTimer]==0)destroyTimer[whichTimer]++;//dont let it be zero

	sprintf(debug_string,"combo:%d",debugDestroyCount);
	VDP_drawText(debug_string,26,0);
}

static void destroyTiles()
{
	u8 timerToUse=0;//the time to destroy is inside each of the members of the array
	while(destroyTimer[timerToUse]==0)timerToUse++;

	if(timer==destroyTimer[timerToUse])
	{
		for (u8 incN=1;incN<MaxInOneMove;incN++)
		{
			board[toDestroyX[timerToUse][incN]][toDestroyY[timerToUse][incN]]=0;
			toDestroyX[timerToUse][incN]=0;
			toDestroyY[timerToUse][incN]=0;
			//if(toDestroyX[timerToUse][incN+1]==0)break;
		}
		destroyTimer[timerToUse]=0;
		p1.flag_redraw=1;
		gravity_delay=GRAVITY_DELAY_AMOUNT;
		//doGravity(0);
	}
}