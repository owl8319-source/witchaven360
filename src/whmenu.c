/*********************************************************************
*
*   WHMENU.C - menu code for Witchaven game
*
*********************************************************************/

#include "icorp.h"
#ifdef WITCHAVEN360
#include "wh_pad.h"
#include "wh_xam.h"
#endif
#include "osd.h"


int loopinstuff; //here it is
extern int vampiretime;

//JSA 4_27 cart and elevator sound variables
extern int  cartsnd,gratesnd,lavasnd,batsnd;

extern int gameactivated;
extern int escapetomenu;

extern int difficulty;
extern int mapon;

extern int currweapon;
extern int selectedgun;
extern int currentpotion;
extern int helmettime;
extern int shadowtime;
extern int nightglowtime;
extern int strongtime;
extern int invisibletime;
extern int manatime;
extern int thunderflash;
extern int thundertime;
extern int currentorb;
extern int currweaponfired;
extern int currweaponanim;
extern int currweaponattackstyle;
extern int currweaponflip;

int goreon=1;
int loadedgame=0;
int loadgo=0;

char typemessage[162];
int typemessageleng = 0, typemode = 0;

char scantoasc[128] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,
    'q','w','e','r','t','y','u','i','o','p','[',']',0,0,'a','s',
    'd','f','g','h','j','k','l',';',39,'`',0,92,'z','x','c','v',
    'b','n','m',',','.','/',0,'*',0,32,0,0,0,0,0,0,
    0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1',
    '2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

char scantoascwithshift[128] = {
    0,0,'!','@','#','$','%','^','&','*','(',')','_','+',0,0,
    'Q','W','E','R','T','Y','U','I','O','P','{','}',0,0,'A','S',
    'D','F','G','H','J','K','L',':',34,'~',0,'|','Z','X','C','V',
    'B','N','M','<','>','?',0,'*',0,32,0,0,0,0,0,0,
    0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1',
    '2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};


extern char boardname[];
extern char loadgamename[];

extern char tempbuf[];
extern int frames;
#define MAXSAVEDGAMES 5

struct savedgame {
     char name[20];
};

#ifdef WITCHAVEN360
// Witchaven 360: and a sixth, the quick save's (wh360_quick), not in the menu's five.
#define WH360_QUICKSLOT MAXSAVEDGAMES
struct savedgame savedgamenames[MAXSAVEDGAMES+1];
#else
struct savedgame savedgamenames[MAXSAVEDGAMES];
#endif


static void menubackground(void) {
    clearallviews(0);
    if (svga)
        svgafullscreenpic(SVGAMENU, SVGAMENU2);

    rotatesprite(svgaxoff,0<<15,svgascale,0,MAINMENU,0,
                0,svgastat,0,0,xdim-1,ydim-1);
}

static void menuwritesprite(int thex, int they, short tilenum, signed char shade,
        char stat, unsigned char dapalnum) {
    (void)stat;
    rotatesprite(svgaxoff+thex*svgascale,they*svgascale,svgascale,0,tilenum,shade,dapalnum,
                 svgaoverstat,0,0,xdim-1,ydim-1);
}


//
// fancy font
//
// to use this function you will need to name the starting letter
// the function will then scan the string and display its chars

char fancy[41] = {  'a','b','c','d','e',
                    'f','g','h','i','j',
                    'k','l','m','n','o',
                    'p','q','r','s','t',
                    'u','v','w','x','y',
                    'z','0','1','2','3',
                    '4','5','6','7','8',
                    '9','!','?','-',':',' ' };


void fancyfont(int x, int y, short tilenum, char *string, char pal) {

     int i, j;
     int len;
     int incr=0;
     int number;
     char temp[40];

     Bstrlwr(string);
     len=(int)strlen(string);
     strcpy(temp,string);

     for(i=0;i<len;i++) {
        tempbuf[i]=temp[i];
        for(j=0;j<40;j++) {
            if(tempbuf[i]==fancy[j]) {
                number=j;
            }
        }
        if( i == 0 ) {
            menuwritesprite(x,y,tilenum+number,0,0,pal);
            incr+=tilesizx[tilenum+number]+1;
        }
        else if( tempbuf[i] != ' ' ) {
            menuwritesprite(x+incr,y,tilenum+number,0,0,pal);
            incr+=tilesizx[tilenum+number]+1;
        }
        else
            incr+=8;
     }

}



void fancyfontscreen(int x, int y, short tilenum, char *string, char pal) {

     int i, j;
     int len;
     int incr=0;
     int number;
     char temp[40];

     Bstrlwr(string);
     len=(int)strlen(string);
     strcpy(temp,string);

     for(i=0;i<len;i++) {
        tempbuf[i]=temp[i];
        for(j=0;j<40;j++) {
            if(tempbuf[i]==fancy[j]) {
                number=j;
            }
        }
        if( i == 0 ) {
            rotatesprite(x<<16,y<<16,65536L,0,tilenum+number,0,pal,
                         2+8+16,0,0,xdim-1,ydim-1);
            incr+=tilesizx[tilenum+number]+1;
        }
        else if( tempbuf[i] != ' ' ) {
            rotatesprite((x+incr)<<16,y<<16,65536L,0,tilenum+number,0,pal,
                         2+8+16,0,0,xdim-1,ydim-1);
            incr+=tilesizx[tilenum+number]+1;
        }
        else
            incr+=8;
     }
}

void fancyfontperm(int x, int y, short tilenum, char *string, char pal) {

     int i, j;
     int len;
     int incr=0;
     int number;
     char temp[40];

     Bstrlwr(string);
     len=(int)strlen(string);
     strcpy(temp,string);

     for(i=0;i<len;i++) {
        tempbuf[i]=temp[i];
        for(j=0;j<40;j++) {
            if(tempbuf[i]==fancy[j]) {
                number=j;
            }
        }
        if( i == 0 ) {
            rotatesprite(x<<16,y<<16,65536L,0,tilenum+number,0,pal,
                         2+8+16+128,0,0,xdim-1,ydim-1);
            incr+=tilesizx[tilenum+number]+1;
        }
        else if( tempbuf[i] != ' ' ) {
            rotatesprite((x+incr)<<16,y<<16,65536L,0,tilenum+number,0,pal,
                         2+8+16+128,0,0,xdim-1,ydim-1);
            incr+=tilesizx[tilenum+number]+1;
        }
        else
            incr+=8;
     }
}

void svgafullscreenpic(short pic1, short pic2) {
    rotatesprite(svgaxoff,0<<15,svgascale,0,pic1,0,
                0,8+16+64,0,0,xdim-1,ydim-1);
    rotatesprite(svgaxoff,ydim<<15,svgascale,0,pic2,0,
                0,8+16+64,0,0,xdim-1,ydim-1);
}

int menuscreen(struct player *plr) {

    struct {
        int x;
        int y;
    }redpic[5] = {
                        { 142,58 },
                        { 140,80 },
                        { 127,104 },
                        { 184,126 },
                        { 183,150 } };

    int exit=0;
    int select=0;
    short redpicnum;
    int goaltime;
#ifdef WITCHAVEN360
    // Witchaven 360: the controller's Y is the keyboard's "/" here (xenon/wh_pad.c).
    const char settingsstr[] = "Press Y for settings";
#else
    const char settingsstr[] = "Press / for settings";
#endif
    void configmenu(void);


    if (netgame) {
        return(0);
//        netkillme();
    }

//JSA BLORB
    SND_CheckLoops();

    redpicnum=NEWGAMEGREEN;

    keystatus[1]=keystatus[0x1c]=keystatus[0x9c]=0;

    //goaltime=totalclock-20L;
    goaltime=totalclock+10L;

    if(svga) {
        svgascale = scale(65536L, ydim, 480);
        svgaxoff = (xdim<<15) - scale(640, ydim<<15, 480);
        svgastat = 8+16+64;
        svgaoverstat = 8+16;
    }
    else {
        svgascale = 65536;
        svgaxoff = 0;
        svgastat = 2+8+16+64;
        svgaoverstat = 2+8+16;
    }

    flushperms();

    ready2send = 0;
    while( !exit ) {
        handleevents();
        OSD_DispatchQueued();
        menubackground();
        menuwritesprite(127,58,MENUSELECTIONS,0,0,0);

        if( select < 5) {
            redpicnum=NEWGAMEGREEN+select;
            menuwritesprite(redpic[select].x,redpic[select].y,redpicnum,0,0,0);
        }
#ifdef WITCHAVEN360
        // Witchaven 360: a line up, to leave the bottom right for the port's version.
        printext256(xdim-(svgaxoff>>16)-8*sizeof(settingsstr)-18+2,ydim-36+2,2,-1,settingsstr,2);
        printext256(xdim-(svgaxoff>>16)-8*sizeof(settingsstr)-18,ydim-36,29,-1,settingsstr,2);
        wh360_drawversion();
#else
        printext256(xdim-(svgaxoff>>16)-8*sizeof(settingsstr)-18+2,ydim-18+2,2,-1,settingsstr,2);
        printext256(xdim-(svgaxoff>>16)-8*sizeof(settingsstr)-18,ydim-18,29,-1,settingsstr,2);
#endif

        nextpage();

            if( totalclock >= goaltime ) {
                if( keystatus[LDN] || keystatus[RDN] ) {
                    TEMPSND();
                    select++;
                    if ( select > 4 )
                        select=0;
                    goaltime=totalclock+15L;
                }
                if( keystatus[LUP] || keystatus[RUP] ) {
                    TEMPSND();
                    select--;
                    if ( select < 0 )
                        select=4;
                    goaltime=totalclock+15L;
                }
                if( keystatus[1] > 0) {
                    TEMPSND();
                    if(gameactivated == 1) {
                        //select=5;
                        totalclock = ototalclock;
                        exit=1;
                    }
                    else
                        select=4;
                    keystatus[1]=0;
                }
                if( keystatus[0x35] ) {
                    keystatus[0x35]=0;
                    configmenu();
                }
                if( keystatus[0x1c] > 0 || keystatus[0x9c] > 0) {
                    keystatus[0x1c]=keystatus[0x9c]=0;keystatus[1]=0;
                    switch(select) {
                    case 0:                 // ok
                        gameactivated=0;
                        //JSA BLORB
                        SND_Sting(S_STING1);
                        SND_FadeMusic();
                        srand(totalclock&30000);
                        //tille maps are finished
                        //if(loadedgame == 0 && netgame == 0)
                        //   mapon=1;
#if 0
                        if (netgame) {
                            netpickmonster();
                            exit=1;
                            break;
                        }
#endif
                        startnewgame(plr);
                        gameactivated=1;
                        exit=1;
                        keystatus[0x1c]=keystatus[0x9c]=0;keystatus[1]=0;
                    break;
                    case 1:
                        TEMPSND();
                        loadsave(plr);
                        if(loadgo == 1)
                            exit=1;
                            loadgo=0;
                        keystatus[0x1c]=keystatus[0x9c]=0;keystatus[1]=0;
                    break;
                    case 2:
                        TEMPSND();
                        thedifficulty();
                        keystatus[0x1c]=keystatus[0x9c]=0;keystatus[1]=0;
                    break;
                    case 3:
                        TEMPSND();
                        help();
                    break;
                    case 4:
                        TEMPSND();
                        quit();
                    break;
                    case 5:
                        totalclock=totalclock;
                        exit=1;
                    break;
                    } // switch
                } // if
            } // the delay
      } // while
    ready2send = 1;

    escapetomenu=0;

//      if (netgame) {
//          netreviveme();
//      }

return(0);
}

//JUNE8
#define  MAXHELPSCREENS   10

void help(void) {

    struct {
        int helpnames;
    }thenames[MAXHELPSCREENS] = {
        { WEAPONS },
        { SPELLS },
        { POTIONS },
        { WALKING },
        { FLYING },
        { CREDIT1 },
        { CREDIT2 },
        { CREDIT3 },
        { CREDIT4 },
        { BETAPAGE }
    };

    int select=0;
    int goaltime = totalclock+10;
    int exit=0;

    while (!exit) {
        handleevents();
#ifdef WITCHAVEN360
        // The pages on the controls, with the controller's buttons in place of the PC's keys (xenon/wh_menu360.c).
        if (select < wh360_helppages())
            wh360_helppage(select);
        else
#endif
        {
        menubackground();
        menuwritesprite(0,0,thenames[select].helpnames,0,0,0);
        }
        nextpage();

        if(totalclock >= goaltime) {
                if( keystatus[LDN]
                    || keystatus[keys[KEYRIGHT]]
                    || keystatus[RDN]
                    || keystatus[keys[KEYBACK]]
                    || keystatus[RRIGHT]) {
                    TEMPSND();
                    select++;
                    if ( select > 9 )
                        select=9;
                    goaltime=totalclock+15L;
                }
                if( keystatus[LUP]
                    || keystatus[keys[KEYLEFT]]
                    || keystatus[RUP]
                    || keystatus[keys[KEYFWD]]
                    || keystatus[RLEFT]) {
                    TEMPSND();
                    select--;
                    if ( select < 0 )
                        select=0;
                    goaltime=totalclock+15L;
                }
                if( keystatus[1] > 0) {
                    exit=1;
                    keystatus[1]=0;
                }
                if( keystatus[0x1c] > 0 || keystatus[0x9c] > 0) {
                    exit=2;
                    keystatus[0x1c]=keystatus[0x9c]=0;
                }
        }
    }
    keystatus[1]=keystatus[0x1c]=keystatus[0x9c]=0;

}

//JUNE6
void loadsave(struct player *plr) {

    int exit=0;
    int select=0;
    int goaltime;

    goaltime=totalclock+10L;

    while ( !exit ) {
        handleevents();
        menubackground();
        if(select == 0) {
            menuwritesprite(182,80,LOADGREEN,0,0,0);
            menuwritesprite(182,119,SAVERED,0,0,0);
        }
        else {
            menuwritesprite(182,80,LOADRED,0,0,0);
            menuwritesprite(182,119,SAVEGREEN,0,0,0);
        }
        nextpage();

        if( totalclock >= goaltime ) {
                if( keystatus[LDN] || keystatus[RDN] ) {
                    TEMPSND();
                    select=1;
                    goaltime=totalclock+15L;
                }
                if( keystatus[LUP] || keystatus[RUP] ) {
                    TEMPSND();
                    select=0;
                    goaltime=totalclock+15L;
                }
                if( keystatus[1] > 0) {
                    exit=1;
                    keystatus[1]=0;
                }
                if( keystatus[0x1c] > 0 || keystatus[0x9c] > 0) {
                    exit=2;
                    keystatus[0x1c]=keystatus[0x9c]=0;
                }
            }
    }

    keystatus[1]=0;

    if(exit == 2)
        switch(select){
        case 0:                 // ok
            loadgame(plr);
        break;
        case 1:                 // ok
            if( gameactivated == 1)
                savegame(plr);
        break;
        }

}

//JUNE6
void quit(void) {

    int exit=0;

    while( !exit ) {
        handleevents();
        menubackground();
        menuwritesprite(123,79,AREYOUSURE,0,0,0);
        nextpage();

        if ( keystatus[0x9c] > 0 || keystatus[0x1c] > 0 || keystatus[0x15] > 0 ) {
            exit=1;
            keystatus[0x1c]=keystatus[0x9c]=0;
        }
        if ( keystatus[1] > 0 || keystatus[0x31] > 0 ) {
            exit=2;
            keystatus[1]=0;
        }
    }

    if ( exit==2 ) {
        keystatus[1]=0;
    }
    else {
        if(svga) {
            exit=0;
            while( !exit ){
                handleevents();
                svgafullscreenpic(SVGAORDER1, SVGAORDER2);
                nextpage();

                if(keystatus[0x39] > 0 || keystatus[1] > 0)
                    exit=1;
            }
            keystatus[0x39]=0;
            keystatus[1]=0;
        }
        else {
            keystatus[0x39]=0;
            keystatus[1]=0;
            exit=0;
            while( !exit ){
                handleevents();
                menuwritesprite(0,0,ORDER1,0,0,0);
                nextpage();

                if(keystatus[0x39] > 0 || keystatus[1] > 0)
                    exit=1;
            }
            keystatus[0x39]=0;
            keystatus[1]=0;
            exit=0;
            /*
            while( !exit ){
                if(keystatus[0x39] > 0 || keystatus[1] > 0)
                    exit=1;
                overwritesprite(0,0,ORDER2,0,0,0);
                nextpage();
            }
            keystatus[0x39]=0;
            keystatus[1]=0;
            exit=0;
            */
        }
        shutdowngame();
    }

}


void thedifficulty(void) {

        struct {
            int x;
            int y;
        }redpic[4]={
                        { 148,146 },
                        { 181,146 },
                        { 215,144 },
                        { 257,143 } };

        int exit=0;
        int selected;
        int select;
        int select2=0;
        int select3=goreon;
        int redpicnum;
        int pickone=0;
        int goaltime;

        select=difficulty-1;
        keystatus[1]=0;
        keystatus[0x1c]=0;
        keystatus[0x9c]=0;

        //loadtile(MAINMENU);

        goaltime=totalclock+10L;

        while ( !exit ) {
            handleevents();

        if( totalclock >= goaltime ) {

            if(keystatus[LUP] > 0 || keystatus[RUP] > 0) {
                TEMPSND();
                select2=0;
                goaltime=totalclock+15L;
            }
            if(keystatus[LDN] > 0 || keystatus[RDN] > 0) {
                TEMPSND();
                select2=1;
                goaltime=totalclock+15L;
            }
            if(select2 == 0)
                pickone=0;
            else
                pickone=1;

            redpicnum=HORNYSKULL1+select;

            menubackground();

            if(select2 == 0) {
                menuwritesprite(148,114,DIFFICULTRED,0,0,0);
                menuwritesprite(127,58,BLOODGOREGREEN,0,0,0);
            }
            else {
                menuwritesprite(148,114,DIFFICULTGREEN,0,0,0);
                menuwritesprite(127,58,BLOODGORERED,0,0,0);
            }
            menuwritesprite(147,143,HORNYBACK,0,0,0);
            menuwritesprite(redpic[select].x,redpic[select].y,redpicnum,0,0,0);

            if( goreon == 1 ) {
                menuwritesprite(180,84,NOGORESHADOW,0,0x04,0);
                menuwritesprite(214,81,GORESOLID,0,0,0);
            }
            else {
                menuwritesprite(180,84,NOGORESOLID,0,0,0);
                menuwritesprite(214,81,GORESHADOW,0,0x04,0);
            }
            nextpage();

            if( pickone == 1 ) {
                if( keystatus[LLEFT] > 0 || keystatus[RLEFT] > 0) {
                    TEMPSND();
                    select--;
                    if( select < 0)
                        select=0;
                    goaltime=totalclock+15L;
                }
                if( keystatus[LRIGHT] > 0 || keystatus[RRIGHT] > 0) {
                    TEMPSND();
                    select++;
                    if( select > 3)
                        select=3;
                    goaltime=totalclock+15L;
                }
                selected=select;
            }
            else {
                if( keystatus[LLEFT] > 0 || keystatus[RLEFT] > 0) {
                    TEMPSND();
                    select3=0;
                    goaltime=totalclock+15L;
                }
                if( keystatus[LRIGHT] > 0 || keystatus[RRIGHT] > 0) {
                    TEMPSND();
                    select3=1;
                    goaltime=totalclock+15L;
                }
                if(select3 == 0)
                    goreon=0;
                else
                    goreon=1;
            }
            if( keystatus[0x1c] > 0 || keystatus[0x9c] > 0) {
                exit=1;
                keystatus[0x1c]=keystatus[0x9c]=0;
            }
            if( keystatus[1] > 0 ) {
                exit=2;
                keystatus[1]=0;
            }
        } // IF

        } // WHILE

        if ( exit == 1 ) {
            switch(selected) {
            case 0:
            difficulty=1;
            break;
            case 1:
            difficulty=2;
            break;
            case 2:
            difficulty=3;
            break;
            case 3:
            difficulty=4;
            break;
            }
        }


}

void startnewgame(struct player *plr) {

    char tempshow2dsector[MAXSECTORS>>3];
    char tempshow2dwall[MAXWALLS>>3];
    char tempshow2dsprite[MAXSPRITES>>3];
    int i;

    if (netgame) {
        SND_StartMusic(mapon-1);
        goto skip;
    }
    if(loadedgame == 0) {
        sprintf(boardname,"level%d.map",mapon);
        setupboard(boardname);
        initplayersprite();
        cleanup();
        SND_StartMusic(mapon-1);
    }
    else if(loadedgame == 1) {
        loadplayerstuff();
            for(i=0;i<(MAXSECTORS>>3);i++) tempshow2dsector[i]=show2dsector[i];
            for(i=0;i<(MAXWALLS>>3);i++) tempshow2dwall[i]=show2dwall[i];
            for(i=0;i<(MAXSPRITES>>3);i++) tempshow2dsprite[i]=show2dsprite[i];
        setupboard(boardname);
            for(i=0;i<(MAXSECTORS>>3);i++) show2dsector[i]=tempshow2dsector[i];
            for(i=0;i<(MAXWALLS>>3);i++) show2dwall[i]=tempshow2dwall[i];
            for(i=0;i<(MAXSPRITES>>3);i++) show2dsprite[i]=tempshow2dsprite[i];
        loadedgame=0;
        SND_StartMusic(mapon-1);
    }
skip:
    if (plr->screensize < 320)
        permanentwritesprite(0,0,BACKGROUND,0,0,0,319,199,0);

    if (plr->screensize <= 320) {
        permanentwritesprite(0,200-46,NEWSTATUSBAR,0,0,0,319,199,0);
    }
    updatepics();

}

#ifdef WITCHAVEN360
// Witchaven 360: quick save and quick load, asked for by the controller (xenon/wh_pad.c: Back with D-pad up or down,
// or a button given them in Settings > Button layout) and done between frames by the game's loop (whaven.c's
// playloop), as the menu's Load / Save does them: into and from a slot of its own, svgm5.map and svgn5.dat, so the
// menu's five are left alone.
extern char displaybuf[];
extern int displaytime;
static int wh360_quick_asked = 0;

void wh360_quick_ask(int load) {
    wh360_quick_asked = load ? 2 : 1;
}

static void wh360_quick_message(const char *text) {
    strcpy(displaybuf, text);
    displaytime = 360;
}

void wh360_quick(struct player *plr) {
    const int asked = wh360_quick_asked;

    wh360_quick_asked = 0;
    if (!asked || netgame || gameactivated == 0) return;

    if (asked == 1) {
        const short screensize = plr->screensize;
        if (plr->health <= 0) {
            wh360_quick_message("can't save now");
            return;
        }
        strcpy(savedgamenames[WH360_QUICKSLOT].name, "quick save");
        sprintf(tempbuf, "svgm%d.map", WH360_QUICKSLOT);
        saveboard(tempbuf, &plr->x, &plr->y, &plr->z, &plr->ang, &plr->sector);
        savedgamename(WH360_QUICKSLOT);
        plr->screensize = screensize;  // savedgamename saves it as 320
        wh360_quick_message("game saved");
        return;
    }

    if (!savedgamedat(WH360_QUICKSLOT)) {
        wh360_quick_message("no quick save yet");
        return;
    }
    sprintf(boardname, "svgm%d.map", WH360_QUICKSLOT);
    sprintf(loadgamename, "svgn%d.dat", WH360_QUICKSLOT);
    loadedgame = 1;
    gameactivated = 1;
    startnewgame(plr);
    // As playloop does on leaving the menu.
    plr->screensize = xenon_pad_status_bar() ? 320 : 328;
    setviewport(plr->screensize);
    drawbackground();
    plr->z = sector[plr->sector].floorz - (PLAYERHEIGHT << 8);
    wh360_quick_message("game loaded");
}
#endif

void loadgame(struct player *plr) {

    int select=0;
    int exit=0;
    int i;
    int goaltime;

    goaltime=totalclock+10L;

    for(i=0;i<MAXSAVEDGAMES;i++)
        if( !savedgamedat(i))
            strcpy(savedgamenames[i].name,"empty");

    while ( !exit ) {
        handleevents();

        menubackground();
        menuwritesprite(138,84,SAVENUMBERS,0,0,0);
        menuwritesprite(190,48,LOADPIC,0,0,0);

        for(i=0;i<MAXSAVEDGAMES;i++) {
            if( i == select )
                fancyfont(154,81+(i*17),THEFONT,savedgamenames[select].name,7);
            else
                fancyfont(154,81+(i*17),THEFONT,savedgamenames[i].name,0);
        }

        nextpage();

        if( totalclock >= goaltime ) {
            if ( keystatus[LUP] > 0 || keystatus[RUP] > 0) {
                select--;
                if( select < 0 )
                    select=4;
                goaltime=totalclock+15L;
            }
            if ( keystatus[LDN] > 0 || keystatus[RDN] > 0) {
                select++;
                if( select > 4 )
                    select=0;
                goaltime=totalclock+15L;
            }

            if ( keystatus[1] > 0 ) {
                exit=1;
                keystatus[1]=0;
            }

            if ( keystatus[0x1c] > 0 || keystatus[0x9c] > 0) {
                keystatus[0x1c]=keystatus[0x9c]=0;
                if( strcmp(savedgamenames[select].name,"empty") == 0 )
                    exit=0;
                else {
                    exit=2;
                    loadgo=1;
                }
            }
        }
    }

    if ( exit == 2 ) {
        keystatus[1]=keystatus[0x1c]=keystatus[0x9c]=0;
        sprintf(boardname,"svgm%d.map",select);
        sprintf(loadgamename,"svgn%d.dat",select);
        loadedgame=1;
        gameactivated=1;
        startnewgame(plr);
    }


}

void savegame(struct player *plr) {

    int exit=0;
    int i;
    int select=0;
    int goaltime;

    (void)plr;

    for(i=0;i<MAXSAVEDGAMES;i++)
        if( !savedgamedat(i))
            strcpy(savedgamenames[i].name,"EMPTY");

    goaltime=totalclock+10L;

    while ( !exit ) {
        handleevents();

        menubackground();
        menuwritesprite(138,84,SAVENUMBERS,0,0,0);
        menuwritesprite(188,50,SAVEPIC,0,0,0);

        for( i=0;i<MAXSAVEDGAMES;i++) {
            if(i == select)
                fancyfont(154,81+(i*17),THEFONT,savedgamenames[select].name,7);
            else
                fancyfont(154,81+(i*17),THEFONT,savedgamenames[i].name,0);
        }

        nextpage();

        if( totalclock >= goaltime ) {
            if( keystatus[LUP] > 0 || keystatus[RUP] > 0) {
                select--;
                if ( select < 0)
                    select=4;
                goaltime=totalclock+15L;
            }

            if( keystatus[LDN] > 0 || keystatus[RDN] > 0) {
                select++;
                if( select > 4 )
                    select=0;
                goaltime=totalclock+15L;
            }

            if( keystatus[1] > 0 ) {
                exit=1;
                keystatus[1]=0;
            }

            if( keystatus[0x1c] > 0 || keystatus[0x9c] > 0) {
                exit=2;
                typemessageleng = 0;
                keystatus[0x1c]=keystatus[0x9c]=0;
                savegametext(select);
            }
        }
    }

}


void savegametext(int select) {

    struct player *plr;

    int exit=0;
    int i, j;
    char tempbuf[40];
    char temp[40];

    int  typemessageleng=0;

    plr=&player[pyrn];

    keystatus[0x1c]=keystatus[0x9c]=0;

    for(i=0;i<128;i++)
        keystatus[i]=0;

    strcpy(temp,"-");

#ifdef WITCHAVEN360
    // Witchaven 360: the name from the console's keyboard (xenon/wh_xam.c), the save screen drawn behind it while
    // it's up. Its name to start with: the save's own, or "save" and its number. Cancelled, nothing is saved; if the
    // keyboard can't open, the save takes that name.
    {
        char name[11];
        int opened, result = 1;

        if (strcmp(savedgamenames[select].name,"empty") && strcmp(savedgamenames[select].name,"EMPTY")) {
            strncpy(name,savedgamenames[select].name,sizeof(name)-1);
            name[sizeof(name)-1]='\0';
        }
        else snprintf(name,sizeof(name),"save %d",select+1);

        opened = wh360_keyboard_start(name, (int)sizeof(name)-1);
        while (opened && (result = wh360_keyboard_poll(name, (int)sizeof(name))) == 0) {
            handleevents();

            menubackground();
            menuwritesprite(138,84,SAVENUMBERS,0,0,0);
            menuwritesprite(188,50,SAVEPIC,0,0,0);
            for(i=0;i<MAXSAVEDGAMES;i++) {
                strcpy(tempbuf, i == select ? "-" : savedgamenames[i].name);
                fancyfont(154,81+(i*17),THEFONT,tempbuf,i == select ? 7 : 0);
            }
            nextpage();
        }

        if (result < 0) {
            exit=1;
        }
        else {
            if (!name[0]) snprintf(name,sizeof(name),"save %d",select+1);
            strcpy(savedgamenames[select].name,name);
            sprintf(tempbuf,"svgm%d.map",select);
            saveboard(tempbuf,&plr->x,&plr->y,&plr->z,&plr->ang,&plr->sector);
            savedgamename(select);
            exit=2;
        }
        keystatus[1]=keystatus[0x1c]=keystatus[0x9c]=0;
        (void)j; (void)temp; (void)typemessageleng;
    }
#else
    while( !exit ) {
        handleevents();

        menubackground();
        menuwritesprite(138,84,SAVENUMBERS,0,0,0);
        menuwritesprite(188,50,SAVEPIC,0,0,0);

        for(i=0;i<MAXSAVEDGAMES;i++) {
            if( i == select ) {
                strcpy(tempbuf,temp);
                fancyfont(154,81+(i*17),THEFONT,tempbuf,7);
            }
            else {
                strcpy(tempbuf,savedgamenames[i].name);
                fancyfont(154,81+(i*17),THEFONT,tempbuf,0);
            }
        }
        nextpage();

        if (keystatus[0xe] > 0) {  // backspace
            if (typemessageleng > 0) {
                temp[typemessageleng]='\0';
                typemessageleng--;
                temp[typemessageleng]='\0';
            }
            else {
                strcpy(temp,"-");
                typemessageleng=0;
            }
            keystatus[0xe]=0;
        }
        if(typemessageleng < 10) {
            for(i=0;i<128;i++) {
                if(keystatus[i] > 0
                    && keystatus[0x0e] == 0    // keypressed not
                    && keystatus[1] == 0       //            esc
                    && keystatus[0x1c] == 0    //            enter l or r
                    && keystatus[0x9c] == 0) { //            backspace
                        for(j=0;j<41;j++) {
                            if(scantoasc[i] == ' ') {
                                continue;
                            }
                            else if(scantoasc[i] == fancy[j]) {
                                temp[typemessageleng]=fancy[j];
                                typemessageleng++;
                                temp[typemessageleng]='\0';
                                keystatus[i]=0;
                            }
                            else
                                keystatus[i]=0;
                        }
                }
            }
        }
        if( keystatus[1] > 0) {
            exit=1;
            keystatus[1]=0;
        }
        if ( keystatus[0x1c] > 0 || keystatus[0x9c] > 0) {
            if( typemessageleng > 0 ) {
                strcpy(savedgamenames[select].name,temp);
                sprintf(tempbuf,"svgm%d.map",select);
                saveboard(tempbuf,&plr->x,&plr->y,&plr->z,&plr->ang,&plr->sector);
                savedgamename(select);
            }
            else {
                sprintf(tempbuf,"svgm%d.map",select);
                saveboard(tempbuf,&plr->x,&plr->y,&plr->z,&plr->ang,&plr->sector);
                savedgamename(select);
            }
            exit=2;
            keystatus[0x1c]=keystatus[0x9c]=0;
        }
    }

#endif

    if( exit == 2 ) {
        keystatus[0x1c]=keystatus[0x9c]=0;
    }

}


int savedgamename(int gn) {

    struct player *plr;
    int  file;
    int  i;
    int tmpanimateptr[MAXANIMATES];

    plr=&player[0];

    plr->screensize=320;

    sprintf(tempbuf,"svgn%d.dat",gn);
    file = open(tempbuf,O_BINARY|O_TRUNC|O_CREAT|O_WRONLY,S_IREAD|S_IWRITE );

    if (file != -1)
    {
        write(file,savedgamenames[gn].name,sizeof(struct savedgame));
        write(file,&player[0],sizeof(struct player));
        write(file,&currweapon,sizeof(currweapon));
        write(file,&selectedgun,sizeof(selectedgun));
        write(file,&currentpotion,sizeof(currentpotion));
        write(file,&helmettime,sizeof(helmettime));
        write(file,&shadowtime,sizeof(shadowtime));
        write(file,&nightglowtime,sizeof(nightglowtime));
        write(file,&visibility,sizeof(visibility));
        write(file,&strongtime,sizeof(strongtime));
        write(file,&invisibletime,sizeof(invisibletime));
        write(file,&manatime,sizeof(manatime));
        write(file,&thunderflash,sizeof(thunderflash));
        write(file,&thundertime,sizeof(thundertime));

        write(file,&currentorb,sizeof(currentorb));
        write(file,&currweaponfired,sizeof(currweaponfired));
        write(file,&currweaponanim,sizeof(currweaponanim));
        write(file,&currweaponattackstyle,sizeof(currweaponattackstyle));
        write(file,&currweaponflip,sizeof(currweapon));
        write(file,&mapon,sizeof(mapon));

        write(file,&totalclock,sizeof(totalclock));
        write(file,&ototalclock,sizeof(ototalclock));
        write(file,&lockclock,sizeof(lockclock));

        //Warning: only works if all pointers are in sector structures!
        for(i=MAXANIMATES-1;i>=0;i--)
            tmpanimateptr[i] = (int)((intptr_t)animateptr[i]-(intptr_t)sector);
        write(file,tmpanimateptr,MAXANIMATES<<2);

        write(file,animategoal,MAXANIMATES<<2);
        write(file,animatevel,MAXANIMATES<<2);
        write(file,&animatecnt,4);

        write(file,show2dsector,MAXSECTORS>>3);
        write(file,show2dwall,MAXWALLS>>3);
        write(file,show2dsprite,MAXSPRITES>>3);
        write(file,&automapping,1);
        write(file,&shieldpoints,2);
        write(file,&vampiretime,2);
        write(file,&poisoned,2);
        write(file,&poisontime,2);

        close(file);
        return 1;
    }
    else
        return 0;



}

int savedgamedat(int gn) {

    int     fh=0;
    char    fname[20];
    char    fsname[20];
    ssize_t nr=0;

    sprintf(fname,"svgm%d.map",gn);

    sprintf(fsname,"svgn%d.dat",gn);

    if( access(fname, F_OK)!=0 )
        return 0;

    if( access(fsname, F_OK)!=0 )
        return 0;

    fh = open(fsname, O_RDONLY | O_BINARY);

    if( fh<0 )
        return 0;

    nr=read(fh, savedgamenames[gn].name, sizeof(struct savedgame));

    close(fh);

    if( nr != sizeof(struct savedgame) )
        return 0;

    return 1;

}

void loadplayerstuff(void) {

    int fh;
    int i;
    int tmpanimateptr[MAXANIMATES];

    fh=open(loadgamename, O_RDONLY | O_BINARY);

    read(fh, savedgamenames[0].name, sizeof(struct savedgame));
    read(fh, &player[0],sizeof(struct player));
    read(fh,&currweapon,sizeof(currweapon));
    read(fh,&selectedgun,sizeof(selectedgun));
    read(fh,&currentpotion,sizeof(currentpotion));
    read(fh,&helmettime,sizeof(helmettime));
    read(fh,&shadowtime,sizeof(shadowtime));
    read(fh,&nightglowtime,sizeof(nightglowtime));
    read(fh,&visibility,sizeof(visibility));
    read(fh,&strongtime,sizeof(strongtime));
    read(fh,&invisibletime,sizeof(invisibletime));
    read(fh,&manatime,sizeof(manatime));
    read(fh,&thunderflash,sizeof(thunderflash));
    read(fh,&thundertime,sizeof(thundertime));

    read(fh,&currentorb,sizeof(currentorb));
    read(fh,&currweaponfired,sizeof(currweaponfired));
    read(fh,&currweaponanim,sizeof(currweaponanim));
    read(fh,&currweaponattackstyle,sizeof(currweaponattackstyle));
    read(fh,&currweaponflip,sizeof(currweaponflip));
    read(fh,&mapon,sizeof(mapon));

    read(fh,&totalclock,sizeof(totalclock));
    read(fh,&ototalclock,sizeof(ototalclock));
    read(fh,&lockclock,sizeof(lockclock));

        //Warning: only works if all pointers are in sector structures!
    read(fh,tmpanimateptr,MAXANIMATES<<2);

    for(i=MAXANIMATES-1;i>=0;i--)
        animateptr[i] = (int *)(tmpanimateptr[i]+(intptr_t)sector);

    read(fh,animategoal,MAXANIMATES<<2);
    read(fh,animatevel,MAXANIMATES<<2);
    read(fh,&animatecnt,4);

    read(fh,show2dsector,MAXSECTORS>>3);
    read(fh,show2dwall,MAXWALLS>>3);
    read(fh,show2dsprite,MAXSPRITES>>3);
    read(fh,&automapping,1);
    read(fh,&shieldpoints,2);
    read(fh,&vampiretime,2);
    read(fh,&poisoned,2);
    read(fh,&poisontime,2);

    close(fh);

}

/***************************************************************************
 *   screen effects for TEKWAR follow...fades, palette stuff, etc..        *
 *                                                                         *
 *                                                     12/15/94 Jeff S.    *
 ***************************************************************************/



#define   MESSIAHRANDOMKEY    215
#define   LEDRANDOMKEY        10

char      forcemessiah=0;
int      messcnt;
int      ledcnt;
char      ledflash;
char      tremors;
char      dofadein=0;

#define   DEADTIME       2   // # minutes of nothing happening
#define   FLASHINTERVAL  20

int      passlock,lastastep,lastbstep,astep,bstep;

extern char pee;
extern char flashflag;

void screenfx(struct player *plr) {
    (void)plr;

 return;
}


#define   MAXSTRLEN      30

//
// (extern asm template)
//
// extern int near asm_main (int *parmeter1, int parameter2);
// Extern must be EXACT prototype.
//
//  #pragma aux asm_main "*_"             /* Define the auxilary function. */ \
//    parm caller     [eax] [ebx]         /* Calling parameters.           */ \
//    value           [eax]               /* Return value.                 */ \
//    modify          [ecx]; // Non parameter registers that will be altered.

void
clearpal(void)
{
    debugprintf("clearpal()\n");
}


char  palette1[256][3],palette2[256][3];

void
getpalette(char *palette)
{
    (void)palette;
    debugprintf("getpalette()\n");
}

void
fillpalette(int red, int green, int blue)
{
    (void)red; (void)green; (void)blue;
    debugprintf("fillpalette()\n");
}


char      foggy=0;

void
fadeout(int start, int end, int red, int green, int blue, int steps)
{
    int      i,j,orig,delta;
    char      *origptr,*newptr;

    getpalette(&palette1[0][0]);
    memcpy(palette2,palette1,768);

    for( i=0; i<steps; i++ ) {
        origptr=&palette1[start][0];
        newptr=&palette2[start][0];
        for( j=start; j<=end; j++ ) {
            orig = *origptr++;
            delta = red-orig;
            *newptr++ = orig + delta * i / steps;
            orig = *origptr++;
            delta = green-orig;
            *newptr++ = orig + delta * i / steps;
            orig = *origptr++;
            delta = blue-orig;
            *newptr++ = orig + delta * i / steps;
        }

        //asmsetpalette(&palette2[0][0]);
    }

      if( foggy == 0 )
        fillpalette(red,green,blue);

 return;
}


void
fadein(int start, int end, int steps)
{
    int  i,j,delta;

      if( steps == 0 ) {
             return;
      }

    getpalette(&palette1[0][0]);
    memcpy(&palette2[0][0],&palette1[0][0],sizeof(palette1));

    start *= 3;
    end = end*3+2;

      // fade through intermediate frames
    for( i=0; i<steps; i++ ) {
        for( j=start; j<=end; j++ ) {
            delta=palette[j]-palette1[0][j];
            palette2[0][j]=palette1[0][j] + delta * i / steps;
        }

        //asmsetpalette(&palette2[0][0]);
    }

      // final color
    //asmsetpalette(palette);

      dofadein=0;
      //clearkeys();
}


void
fog1(void)
{
      if( foggy == 0 ) {
             foggy=1;
             fadeout(1,254,8,8,10,2);
      }
      else {
             foggy=0;
             fadein(0,255,2);
      }
}


void
fog2(void)
{
      unsigned char *lookptr;

      lookptr=palookup[0];
      palookup[0]=palookup[1];
      palookup[1]=lookptr;
}

void
makefxlookups(void)
{
      unsigned char palbuf[256];
      short     i;

      for( i=0; i<256; i++ )
             palbuf[i]=*(palookup[0]+i);

      makepalookup(1, palbuf, 60,60,60,1);
}



#define   NUMWHITESHIFTS      3
#define   WHITESTEPS          20
#define   WHITETICS           6

#define   NUMREDSHIFTS        4
#define   REDSTEPS            8

#define   NUMGREENSHIFTS      4
#define   GREENSTEPS          8

#define   NUMBLUESHIFTS       4
#define   BLUESTEPS           8

unsigned char whiteshifts[NUMREDSHIFTS][768];
unsigned char redshifts[NUMREDSHIFTS][768];
unsigned char greenshifts[NUMGREENSHIFTS][768];
unsigned char blueshifts[NUMBLUESHIFTS][768];

int       redcount,whitecount,greencount,bluecount;
char      palshifted;


void
initpaletteshifts(void)
{
    unsigned char *workptr,*baseptr;
    int       i,j,delta;

    for( i=1; i<=NUMREDSHIFTS; i++ ) {
        workptr=(unsigned char *)&redshifts[i-1][0];
        baseptr=&palette[0];
        for( j=0; j<=255; j++) {
            delta=64-*baseptr;
            *workptr++=*baseptr++ + delta * i / REDSTEPS;
            delta=-*baseptr;
            *workptr++=*baseptr++ + delta * i / REDSTEPS;
            delta=-*baseptr;
            *workptr++=*baseptr++ + delta * i / REDSTEPS;
        }
    }

    for( i=1; i<=NUMWHITESHIFTS; i++ ) {
        workptr=&whiteshifts[i-1][0];
        baseptr=&palette[0];
        for( j=0; j<=255; j++ ) {
            delta = 64-*baseptr;
            *workptr++ = *baseptr++ + delta * i / WHITESTEPS;
            delta = 62-*baseptr;
            *workptr++ = *baseptr++ + delta * i / WHITESTEPS;
            delta = 0-*baseptr;
            *workptr++ = *baseptr++ + delta * i / WHITESTEPS;
        }
    }

    for( i=1; i<=NUMGREENSHIFTS; i++) {
        workptr=&greenshifts[i-1][0];
        baseptr=&palette[0];
        for( j=0; j<=255; j++) {
            delta=-*baseptr;
            *workptr++=*baseptr++ + delta * i / GREENSTEPS;
            delta=64-*baseptr;
            *workptr++=*baseptr++ + delta * i / GREENSTEPS;
            delta=-*baseptr;
            *workptr++=*baseptr++ + delta * i / GREENSTEPS;
        }
    }

    for( i=1; i<=NUMBLUESHIFTS; i++) {
        workptr=&blueshifts[i-1][0];
        baseptr=&palette[0];
        for( j=0; j<=255; j++) {
            delta=-*baseptr;
            *workptr++=*baseptr++ + delta * i / BLUESTEPS;
            delta=-*baseptr;
            *workptr++=*baseptr++ + delta * i / BLUESTEPS;
            delta=64-*baseptr;
            *workptr++=*baseptr++ + delta * i / BLUESTEPS;
        }
    }


 return;
}

void
startgreenflash(int greentime)
{
    greencount=0;

    greencount+=greentime;

    if( greencount < 0 ) {
        greencount=0;
    }

}

void
startblueflash(int bluetime)
{
    bluecount=0;

    bluecount+=bluetime;

    if( bluecount < 0 ) {
        bluecount=0;
    }

}


void
startredflash(int damage)
{

      redcount=0;

      redcount+=damage;

      if( redcount < 0 ) {
             redcount=0;
      }
}

void
startwhiteflash(int bonus)
{
      whitecount = 0;

      whitecount+=bonus;

      if( whitecount < 0 ) {
             whitecount=0;
      }

}


void
updatepaletteshifts(void)
{
      int   red,white,green,blue;

    if (whitecount)
    {
        white = whitecount/WHITETICS +1;
        if (white>NUMWHITESHIFTS)
            white = NUMWHITESHIFTS;
        whitecount -= TICSPERFRAME;
        if (whitecount < 0)
            whitecount = 0;
    }
    else {
        white = 0;
      }

    if (redcount)
    {
        red = redcount/10 +1;
        if (red>NUMREDSHIFTS)
            red = NUMREDSHIFTS;
        redcount -= TICSPERFRAME;
        if (redcount < 0)
            redcount = 0;
    }
    else {
        red = 0;
      }

    if (greencount)
    {
        green = greencount/10 +1;
        if (green>NUMGREENSHIFTS)
            green = NUMGREENSHIFTS;
        greencount -= TICSPERFRAME;
        if (greencount < 0)
            greencount = 0;
    }
    else {
        green = 0;
      }

    if (bluecount)
    {
        blue = bluecount/10 +1;
        if (blue>NUMBLUESHIFTS)
            blue = NUMBLUESHIFTS;
        bluecount -= TICSPERFRAME;
        if (bluecount < 0)
            bluecount = 0;
    }
    else {
        blue = 0;
      }

    if( red ) {
#if USE_POLYMOST && USE_OPENGL
        if (getrendermode() >= 3) setpalettefade(64,0,0,64 * red / REDSTEPS);
        else
#endif
        setbrightness(gbrightness,&redshifts[red-1][0],0);
        palshifted = 1;
    }
    else if( white ) {
#if USE_POLYMOST && USE_OPENGL
        if (getrendermode() >= 3) setpalettefade(64,62,0,64 * white / WHITESTEPS);
        else
#endif
        setbrightness(gbrightness,&whiteshifts[white-1][0],0);
        palshifted = 1;
    }
    else if( green ) {
#if USE_POLYMOST && USE_OPENGL
        if (getrendermode() >= 3) setpalettefade(0,64,0,64 * green / GREENSTEPS);
        else
#endif
        setbrightness(gbrightness,&greenshifts[green-1][0],0);
        palshifted=1;
    }
    else if( blue ) {
#if USE_POLYMOST && USE_OPENGL
        if (getrendermode() >= 3) setpalettefade(0,0,64,64 * blue / BLUESTEPS);
        else
#endif
        setbrightness(gbrightness,&blueshifts[blue-1][0],0);
        palshifted=1;
    }

    else if( palshifted ) {
#if USE_POLYMOST && USE_OPENGL
        if (getrendermode() >= 3) setpalettefade(0,0,0,0);
        else
#endif
        setbrightness(gbrightness,palette,0);  // back to normal
        palshifted = 0;
    }

 return;
}


void
finishpaletteshifts(void)
{
    if( palshifted == 1 ) {
        palshifted = 0;
        //asmsetpalette(&palette[0]);
    }

 return;
}

#if 0
void clearkeys(void) {

      memset(keystatus, 0, sizeof(keystatus));

 //return;
}
#endif


//JUST USING THIS FOR NOW
void TEMPSND(void) {
SND_Sound( rand()%60 );
}

void cleanup(void) {

    if( difficulty > 1 ) {
        currweapon=1;
        selectedgun=1;
    }
    else {
        currweapon=4;
        selectedgun=4;
    }
    currentpotion=0;
    helmettime=-1;
    shadowtime=-1;
    nightglowtime=-1;
    strongtime=-1;
    invisibletime=-1;
    manatime=-1;
    currentorb=0;
    currweaponfired=3;
    currweaponanim=0;
    currweaponattackstyle=0;
    currweaponflip=0;

}


enum configverb {
    init = 0,
    increase = 1,
    decrease = -1,
    draw = -2,
    activate = -3,
};

static const char *configitem_setvolume(enum configverb verb, int source) {
    static char digistr[16]={0}, midistr[16]={0};
    switch (verb) {
        case increase:
        case decrease: {
            int vol = source == DIGI ? digilevel : musiclevel;
            vol = max(1, min(16, vol + (int)verb));
            SND_Mixer(source, vol);
            if (source == DIGI) {
                digilevel = vol;
                SND_Sound(10);
                digistr[0]=0;
            } else {
                musiclevel = vol;
                midistr[0]=0;
            }
            break;
        }
        case draw:
            if (source == DIGI) {
                if (!digistr[0]) snprintf(digistr,sizeof(digistr),"%d",digilevel);
                return digistr;
            } else {
                if (!midistr[0]) snprintf(midistr,sizeof(midistr),"%d",musiclevel);
                return midistr;
            }
            break;
        default: break;
    }
    return NULL;
};

static const char *configitem_videomode(enum configverb verb, int param) {
    static char displaystr[64]={0}, colourstr[64]={0}, resstr[64]={0};
    static int sel = -1;
    int rx, ry;
    switch (verb) {
        case init: {
            rx = xres, ry = yres;
            sel = checkvideomode(&rx, &ry, bpp, fullscreen, 0);
            break;
        }
        case increase:
        case decrease: {
            int newsel = sel;
            int vs = validmode[sel].validmodeset;
            switch (param) {
                case 0: // Display.
                    if ((int)verb < 0) vs = validmodeset[vs].prevfsmodeset;
                    else if ((int)verb > 0) vs = validmodeset[vs].nextfsmodeset;
                    if (vs < 0) vs = validmode[newsel].validmodeset;
                    break;
                case 1: // Colour.
                    if ((int)verb < 0) vs = validmodeset[vs].prevbppmodeset;
                    else if ((int)verb > 0) vs = validmodeset[vs].nextbppmodeset;
                    if (vs < 0) vs = validmode[newsel].validmodeset;
                    break;
                case 2: // Resolution.
                    newsel += (int)verb;
                    if (newsel < validmodeset[vs].firstmode) newsel = validmodeset[vs].firstmode;
                    else if (newsel > validmodeset[vs].lastmode) newsel = validmodeset[vs].lastmode;
                    break;
                default: return NULL;
            }

            rx = validmode[newsel].xdim;
            ry = validmode[newsel].ydim;
            newsel = checkvideomode(&rx, &ry, validmodeset[vs].bpp,
                SETGAMEMODE_FULLSCREEN(validmodeset[vs].display, validmodeset[vs].fs), 1);
            if (newsel >= 0) {
                sel = newsel;
                displaystr[0] = colourstr[0] = resstr[0] = 0;
            }
            break;
        }
        case activate: {
            fullscreen = SETGAMEMODE_FULLSCREEN(validmode[sel].display,validmode[sel].fs);
            xdimgame = validmode[sel].xdim;
            ydimgame = validmode[sel].ydim;
            bppgame = validmode[sel].bpp;
            setup3dscreen();
            break;
        }
        case draw:
            switch (param) {
                case 0:
                    if (!displaystr[0]) {
                        if (!validmode[sel].fs) strcpy(displaystr, "Windowed");
                        else if (displaycnt > 1) {
                            int l = snprintf(displaystr, sizeof(displaystr), "%d: %s", validmode[sel].display,
                                getdisplayname(validmode[sel].display));
                            if (l > 12) strcpy(&displaystr[12-3], "...");
                        }
                        else strcpy(displaystr, "Fullscreen");
                    }
                    return displaystr;
                case 1:
                    if (!colourstr[0]) snprintf(colourstr, sizeof(colourstr), "%d-bpp", validmode[sel].bpp);
                    return colourstr;
                case 2:
                    if (!resstr[0]) snprintf(resstr, sizeof(resstr), "%dx%d", validmode[sel].xdim, validmode[sel].ydim);
                    return resstr;
            }
        default: break;
    }
    return NULL;
};

static const char *configitem_setkeys(enum configverb verb, int param) {
    extern const int defaultkeys[NUMKEYS];
    extern const int modernkeys[NUMKEYS];
    if (verb==activate) {
        if (param == 0) memcpy(keys,defaultkeys,sizeof(keys));
        else if (param == 1) memcpy(keys,modernkeys,sizeof(keys));
    }
    return NULL;
};

#ifdef WITCHAVEN360
// Witchaven 360: the controller's pages (xenon/wh_menu360.c).
static const char *configitem_controller(enum configverb verb, int param) {
    if (param == 2) {
        // Invert look: the right stick's up and down the other way round, kept in controller.cfg.
        if (verb==activate || verb==increase || verb==decrease) {
            xenon_pad_set_invert_look(!xenon_pad_invert_look());
            xenon_pad_save_layout();
        }
        else if (verb==draw) return xenon_pad_invert_look() ? "On" : "Off";
        return NULL;
    }
    if (param == 5) {
        // Look sensitivity, 1 to 10.
        int v = xenon_pad_look_sensitivity();
        if (verb==draw) {
            static char text[8];
            snprintf(text, sizeof(text), "%d", v);
            return text;
        }
        if (verb==increase || verb==activate) v = v >= 10 ? (verb==activate ? 1 : 10) : v + 1;
        else if (verb==decrease) v = v > 1 ? v - 1 : 1;
        if (v != xenon_pad_look_sensitivity()) {
            xenon_pad_set_look_sensitivity(v);
            xenon_pad_save_layout();
        }
        return NULL;
    }
    if (param == 6) {
        // Stick dead zone, 0 to 30 per cent by 5s.
        int v = xenon_pad_dead_zone();
        if (verb==draw) {
            static char text[8];
            snprintf(text, sizeof(text), "%d%%", v);
            return text;
        }
        if (verb==increase || verb==activate) v = v >= 30 ? (verb==activate ? 0 : 30) : v + 5;
        else if (verb==decrease) v = v > 0 ? v - 5 : 0;
        if (v != xenon_pad_dead_zone()) {
            xenon_pad_set_dead_zone(v);
            xenon_pad_save_layout();
        }
        return NULL;
    }
    if (param == 7) {
        // Status bar: on, or off for the view to fill the screen (whaven.c's playloop, on leaving the menu).
        if (verb==activate || verb==increase || verb==decrease) {
            xenon_pad_set_status_bar(!xenon_pad_status_bar());
            xenon_pad_save_layout();
        }
        else if (verb==draw) return xenon_pad_status_bar() ? "On" : "Off";
        return NULL;
    }
    if (param == 8) {
        // Smoothing: off, smooth or sharp (xenon/blit.frag).
        static const char *const names[] = { "Off", "Smooth", "Sharp" };
        int v = xenon_pad_smoothing();
        if (verb==draw) return names[v];
        if (verb==increase || verb==activate) v = (v + 1) % 3;
        else if (verb==decrease) v = (v + 2) % 3;
        xenon_pad_set_smoothing(v);
        xenon_pad_save_layout();
        return NULL;
    }
    if (param == 4) {
        // Brightness: the game's own (the PC's F10), 0 to 8, through the palette; kept in whaven.ini.
        int b = gbrightness;
        if (verb==activate || verb==increase) b = b >= 8 ? (verb==activate ? 0 : 8) : b + 1;
        else if (verb==decrease) b = b > 0 ? b - 1 : 0;
        else if (verb==draw) {
            static char text[8];
            snprintf(text, sizeof(text), "%d", gbrightness);
            return text;
        }
        if (b != gbrightness) {
            gbrightness = brightness = b;
            setbrightness(b, palette, 0);
        }
        return NULL;
    }
    if (param == 3) {
        // Vibration: the controller rumbles when hit, striking, casting and picking things up; kept in controller.cfg.
        if (verb==activate || verb==increase || verb==decrease) {
            xenon_pad_set_vibration(!xenon_pad_vibration());
            xenon_pad_save_layout();
        }
        else if (verb==draw) return xenon_pad_vibration() ? "On" : "Off";
        return NULL;
    }
    if (verb==activate) {
        if (param == 0) wh360_buttonlayoutmenu();
        else wh360_controlsmenu();
    }
    return NULL;
}
#endif

static const char *configitem_showosd(enum configverb verb, int param) {
    (void)param;
    if (verb==activate) OSD_ShowDisplay(1);
    return NULL;
}

void configmenu(void) {
    enum menuitemtype {
        spacer,
        inttoggle,
        callback,
    };
    static const struct {
        enum menuitemtype type;
        const char *label;
        union {
            struct {
                int *value;
            } inttoggle;
            struct {
                const char * (*func)(enum configverb,int);
                int param;
            } callback;
        } value;
    } items[] = {
#ifdef WITCHAVEN360
        // Witchaven 360: the volumes, the resolution, and the controller. Not the display or colour depth, the same
        // on the console whatever they say, nor the mouse, the keyboard's keys or the console, with no keyboard.
        {
            .type = callback,
            .label = "Sound volume",
            .value = { .callback = { configitem_setvolume, DIGI } }
        },
        {
            .type = callback,
            .label = "Music volume",
            .value = { .callback = { configitem_setvolume, MIDI } }
        },
        { .type = spacer },
        {
            .type = callback,
            .label = "Resolution",
            .value = { .callback = { configitem_videomode, 2 } }
        },
        {
            .type = callback,
            .label = "Brightness",
            .value = { .callback = { configitem_controller, 4 } }
        },
        {
            .type = callback,
            .label = "Smoothing",
            .value = { .callback = { configitem_controller, 8 } }
        },
        {
            .type = callback,
            .label = "Status bar",
            .value = { .callback = { configitem_controller, 7 } }
        },
        { .type = spacer },
        {
            .type = callback,
            .label = "Look sensitivity",
            .value = { .callback = { configitem_controller, 5 } }
        },
        {
            .type = callback,
            .label = "Stick dead zone",
            .value = { .callback = { configitem_controller, 6 } }
        },
        {
            .type = callback,
            .label = "Invert look",
            .value = { .callback = { configitem_controller, 2 } }
        },
        {
            .type = callback,
            .label = "Vibration",
            .value = { .callback = { configitem_controller, 3 } }
        },
        {
            .type = callback,
            .label = "Button layout",
            .value = { .callback = { configitem_controller, 0 } }
        },
        {
            .type = callback,
            .label = "Controls",
            .value = { .callback = { configitem_controller, 1 } }
        },
#else
        {
            .type = callback,
            .label = "Sound volume",
            .value = { .callback = { configitem_setvolume, DIGI } }
        },
        {
            .type = callback,
            .label = "Music volume",
            .value = { .callback = { configitem_setvolume, MIDI } }
        },
        { .type = spacer },
        {
            .type = callback,
            .label = "Display",
            .value = { .callback = { configitem_videomode, 0 } }
        },
        {
            .type = callback,
            .label = "Colour",
            .value = { .callback = { configitem_videomode, 1 } }
        },
        {
            .type = callback,
            .label = "Resolution",
            .value = { .callback = { configitem_videomode, 2 } }
        },
        { .type = spacer },
        {
            .type = inttoggle,
            .label = "Mouse look toggle",
            .value = { .inttoggle = { &mouselookmode } }
        },
        { .type = spacer },
        {
            .type = callback,
            .label = "Set default keys (arrows movement)",
            .value = { .callback = { configitem_setkeys, 0 } }
        },
        {
            .type = callback,
            .label = "Set modern keys (WASD movement)",
            .value = { .callback = { configitem_setkeys, 1 } }
        },
        { .type = spacer },
        {
            .type = callback,
            .label = "Show console",
            .value = { .callback = { configitem_showosd, 0 } }
        },
#endif
    };

    int delaytime=totalclock+10, item, select=0, y;

    for (item=0; item<(int)Barraylen(items); item++) {
        if (items[item].type == callback)
            (void)items[item].value.callback.func(init, items[item].value.callback.param);
    }

    while (1) {
        handleevents();
        OSD_DispatchQueued();

        if (keystatus[1]) {
            keystatus[1]=0;
            break;
        }
        if (totalclock >= delaytime) {
            enum configverb verb=init;
            if (keystatus[LUP] || keystatus[RUP]) {
                delaytime=totalclock+15;
                do select = max(0, select-1);
                while (select > 0 && items[select].type == spacer);
            }
            if (keystatus[LDN] || keystatus[RDN]) {
                delaytime=totalclock+15;
                do select = min((int)Barraylen(items)-1, select+1);
                while (select < (int)Barraylen(items)-1 && items[select].type == spacer);
            }
            switch (items[select].type) {
                case callback:
                    if (keystatus[LLEFT] || keystatus[RLEFT]) verb=decrease;
                    else if (keystatus[LRIGHT] || keystatus[RRIGHT]) verb=increase;
                    // fallthrough
                case inttoggle:
                    if (keystatus[0x1c]) verb=activate, keystatus[0x1c]=0;
                    break;
                default: break;
            }
            if (verb != init) {
                delaytime=totalclock+20;
                switch (items[select].type) {
                    case inttoggle:
                        *items[select].value.inttoggle.value = !*items[select].value.inttoggle.value;
                        break;
                    case callback:
                        items[select].value.callback.func(verb, items[select].value.callback.param);
                        break;
                    default: break;
                }
            }
        }

        clearallviews(0);
#ifdef WITCHAVEN360
        // Witchaven 360: the picture darker, so the settings read over it, as the controller's pages
        // (xenon/wh_menu360.c).
        if (svga) {
            rotatesprite(svgaxoff,0<<15,svgascale,0,SVGAMENU,16,0,8+16+64,0,0,xdim-1,ydim-1);
            rotatesprite(svgaxoff,ydim<<15,svgascale,0,SVGAMENU2,16,0,8+16+64,0,0,xdim-1,ydim-1);
        }
        rotatesprite(svgaxoff,0<<15,svgascale,0,TITLEPIC,16, 0,svgastat,0,0,xdim-1,ydim-1);
#else
        if (svga) svgafullscreenpic(SVGAMENU, SVGAMENU2);
        rotatesprite(svgaxoff,0<<15,svgascale,0,TITLEPIC,0, 0,svgastat,0,0,xdim-1,ydim-1);
#endif

        y=24;
        for (item=0; item<(int)Barraylen(items); item++) {
            if (items[item].type == spacer) {
                y+=6;
                continue;
            }
            printext256((svgaxoff>>16)+32,y, item==select ? 237 : 29, -1, items[item].label, 2);
            switch (items[item].type) {
                case spacer: break;
                case inttoggle: {
                    int val = *items[item].value.inttoggle.value;
                    printext256((svgaxoff>>16)+200,y, item==select ? 125 : 29, -1, val ? "On":"Off", 2);
                    break;
                }
                case callback: {
                    const char *str = items[item].value.callback.func(draw, items[item].value.callback.param);
                    if (str) printext256((svgaxoff>>16)+200,y, item==select ? 125 : 29, -1, str, 2);
                    break;
                }
            }
            y+=15;
        }

        nextpage();
    }
}
