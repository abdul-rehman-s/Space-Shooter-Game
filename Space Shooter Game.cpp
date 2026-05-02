#include <iostream>
#include <conio.h>
#include <fstream>
#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace std;

// ============================================================
// CONSTANTS
// ============================================================
const int ROWS = 30;
const int COLS = 96;

const int NO_BUL = -99;

const int MOVE_R = 1;
const int MOVE_C = 2;

const int FRAME_MS = 70;
const int POLL_MS = 8;

const int MAX_HP = 10;
const int IFRAMES = 10;

const int KILLS_LVL1 = 10;
const int KILLS_LVL2 = 12;

const int NSTARS = 20;

// ============================================================
// GLOBAL STATE
// ============================================================
char plane[ROWS][COLS + 1];

int pR[3] = {24,25,26};
int pC[5] = {44,45,46,47,48};

int playerHP = MAX_HP;
int iframes = 0;

int bul[3] = {NO_BUL,NO_BUL,NO_BUL};

int kills = 0;
int score = 0;
int level = 1;

bool running = true;
bool gameWon = false;
bool levelTransitioning = false;

// stars
int starR[NSTARS], starC[NSTARS];

// wave
int waveRow = 2;
bool shipAlive[4] = {true,true,true,true};

const int FA_OFF[4] = {0,-3,-1,-4};
const int FA_COL[4] = {6,26,56,76};

const int FB_OFF[4] = {0,-3,2,-5};
const int FB_COL[4] = {8,28,48,72};

bool formationA = true;

int eBulRow = 5;
int eBulCol = -1;

// gift
bool giftVis = false;
int giftRow = 3;

// scores
int topScore[3] = {0,0,0};

// ============================================================
// CONSOLE
// ============================================================
void gotoxy(int x,int y)
{
    COORD c={(SHORT)x,(SHORT)y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),c);
}

void hideCursor()
{
    HANDLE h=GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(h,&ci);
    ci.bVisible=false;
    SetConsoleCursorInfo(h,&ci);
}

// ============================================================
// PLANE
// ============================================================
void clearPlane()
{
    for(int i=0;i<ROWS;i++)
    {
        for(int j=0;j<COLS;j++)
            plane[i][j]=' ';
        plane[i][COLS]='\0';
    }
}

void put(int r,int c,char ch)
{
    if(r>=0 && r<ROWS && c>=0 && c<COLS)
        plane[r][c]=ch;
}

// ============================================================
// STARS
// ============================================================
void initStars()
{
    srand(time(0));
    for(int i=0;i<NSTARS;i++)
    {
        starR[i]=rand()%ROWS;
        starC[i]=rand()%COLS;
    }
}

void drawStars()
{
    for(int i=0;i<NSTARS;i++)
        put(starR[i],starC[i],'.');
}

void scrollStars()
{
    for(int i=0;i<NSTARS;i++)
    {
        starR[i]++;
        if(starR[i]>=ROWS)
        {
            starR[i]=0;
            starC[i]=rand()%COLS;
        }
    }
}

// ============================================================
// PLAYER
// ============================================================
void drawShip()
{
    if(iframes>0 && iframes%2==0) return;

    put(pR[0],pC[2],'^');
    put(pR[1],pC[1],'<');
    put(pR[1],pC[2],'|');
    put(pR[1],pC[3],'>');
    put(pR[2],pC[2],'W');
}

void moveShip(int dr,int dc)
{
    if(pR[0]+dr<1 || pR[2]+dr>=ROWS-1) return;
    if(pC[0]+dc<1 || pC[4]+dc>=COLS-1) return;

    for(int i=0;i<3;i++) pR[i]+=dr;
    for(int i=0;i<5;i++) pC[i]+=dc;
}

// ============================================================
// INPUT
// ============================================================
void fireBullet()
{
    for(int i=0;i<3;i++)
        if(bul[i]==NO_BUL)
        {
            bul[i]=pR[0]-1;
            return;
        }
}

void handleInput()
{
    if(_kbhit())
    {
        int k=_getch();
        if(k==0||k==224) k=_getch();

        switch(k)
        {
            case 72: moveShip(-MOVE_R,0); break;
            case 80: moveShip(MOVE_R,0); break;
            case 75: moveShip(0,-MOVE_C); break;
            case 77: moveShip(0,MOVE_C); break;
            case 32: fireBullet(); break;
            case 27: running=false; break;
        }
    }
}

// ============================================================
// BULLETS
// ============================================================
void moveBullets()
{
    for(int i=0;i<3;i++)
    {
        if(bul[i]==NO_BUL) continue;
        bul[i]--;
        if(bul[i]<1) bul[i]=NO_BUL;
    }
}

bool bulletAt(int r,int c)
{
    for(int i=0;i<3;i++)
    {
        if(bul[i]==NO_BUL) continue;
        int br=bul[i];
        if(br==r && abs(pC[2]-c)<=1)
            return true;
    }
    return false;
}

void drawBullets()
{
    for(int i=0;i<3;i++)
        if(bul[i]!=NO_BUL)
            put(bul[i],pC[2],'*');
}

// ============================================================
// ENEMY
// ============================================================
void drawEnemy(int r,int c)
{
    put(r,c,'#');
    put(r+1,c-1,'[');
    put(r+1,c,'#');
    put(r+1,c+1,']');
}

// ============================================================
// WAVE RESET
// ============================================================
void spawnWave()
{
    waveRow=2;
    eBulRow=5;
    eBulCol=-1;

    for(int i=0;i<4;i++)
        shipAlive[i]=true;

    formationA=!formationA;
}

// ============================================================
// WAVE LOGIC (FIXED)
// ============================================================
void processWave()
{
    const int* OFF = formationA ? FA_OFF : FB_OFF;
    const int* COL = formationA ? FA_COL : FB_COL;

    waveRow++;

    if(level==2 && waveRow%3==0)
        waveRow++;

    if(eBulRow<ROWS-1) eBulRow++;

    if(eBulRow>=ROWS-1)
    {
        int s=rand()%4;
        if(shipAlive[s])
        {
            eBulCol=COL[s];
            eBulRow=waveRow+2;
        }
    }

    if(eBulCol>=0)
        put(eBulRow,eBulCol,'v');

    for(int i=0;i<4;i++)
    {
        if(!shipAlive[i]) continue;

        int sr=waveRow+OFF[i];
        int sc=COL[i];

        drawEnemy(sr,sc);

        if(bulletAt(sr,sc))
        {
            if(!levelTransitioning)
                kills++;

            shipAlive[i]=false;
            score+=10;
        }
    }
}

// ============================================================
// LEVEL SYSTEM (FIXED)
// ============================================================
void checkLevel()
{
    if(!levelTransitioning && level==1 && kills>=KILLS_LVL1)
    {
        levelTransitioning=true;

        system("cls");
        cout<<"\n\n LEVEL 2 START!\n";
        _getch();

        level=2;
        kills=0;
        playerHP=MAX_HP;
        giftVis=true;

        levelTransitioning=false;
    }

    if(!levelTransitioning && level==2 && kills>=KILLS_LVL2)
    {
        gameWon=true;
        running=false;
    }
}

// ============================================================
// GAME LOOP
// ============================================================
void gameLoop()
{
    initStars();
    hideCursor();

    DWORD last=GetTickCount();

    while(running && playerHP>0)
    {
        while(GetTickCount()-last<FRAME_MS)
        {
            handleInput();
            Sleep(POLL_MS);
        }
        last=GetTickCount();

        if(iframes>0) iframes--;

        moveBullets();
        scrollStars();

        if(waveRow>ROWS-5)
            spawnWave();

        processWave();
        checkLevel();

        clearPlane();
        drawStars();
        drawShip();
        drawBullets();

        render:
        gotoxy(0,0);
        for(int i=0;i<ROWS;i++)
            cout<<plane[i]<<endl;
    }

    system("cls");

    if(gameWon)
        cout<<"\nYOU WON!";
    else
        cout<<"\nGAME OVER";

    cout<<"\nScore: "<<score<<endl;
}

// ============================================================
// MAIN
// ============================================================
int main()
{
    system("color 0b");
    gameLoop();
    return 0;
}