#include <windows.h>
#include <shellapi.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
//#include <GL/glext.h>

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <ctime>
#include <cmath>
#include <fstream>
#include <map>
#include <MMSystem.h>

/*

CHEAT CODES:
0      Z     -     Display the dealer's points (even when his 2nd card is hidden)
1      J     -     Attempt to deal yourself a BlackJack
2      B     -     Make the dealer try to bust himself (hit on 20 and lower)
3      Y     -     Give yourself a split
4      M     -     Money Cheat (Adds the amount of your wager to your cash)
*/

using namespace std;

#include "SOIL.h"

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC*, HGLRC*);
void DisableOpenGL(HWND, HDC, HGLRC);

/* Our global variables (hey, this style lends itself to gl/glut!) */
bool Cheats[4];
//int Money = 500; // Stores the money of the player
int Money;
int HighScore;
int CurrentBet = 5; // Stores what the player's bet is set at
int Wager = 5; // If the player wants to tweak with this after a hand, it won't affect the finished hand
bool HitPressed;
bool StayPressed;
bool NewPressed;
bool QuitPressed;
bool DoubleDownPressed;
bool SplitPressed;
bool CannotPlaySound;
int SplitMode; // 0 = nothing 1 = player playing 2 = player done with splits
int NumberOfSplits; // e.g. player dealt two K's, splits the kings and gets a third king, splits third king, this value would be 3
//int SplitPointTotals[11];
bool HandledSplits[11];
int SplitCounter;
bool InsurancePressed;
bool ShowDealerCards; // Activated when player can see dealer's cards
bool ClearBetPressed;
bool Add1Pressed;
bool Add5Pressed;
bool Add25Pressed;
int MouseX;
int MouseY;
float MouseXf;
float MouseYf;
//vector <string> TextureStrings;
//vector <GLuint> TextureIDs;
int PlayerTotal;
int DealerTotal;

map <string, GLuint> Textures;
map <string,GLuint>::iterator iter;

char EndMessage;
static char label[100];
int CurrentLine;

class Card
{
public:
      bool Initialized;
      int Suit; // 0=club 1=diamond 2=heart 3=spade
      int Points; // 0=ace 1=2 ... 9=10 10=J 11=Q 12=K
      string SuitText();
      string PointsText();
      Card();
      Card& operator=(const Card &rhs);
      bool operator==(const Card &rhs);
};

Card::Card()
{
      Initialized = false;
      Suit = 0;
      Points = 0;
}

string Card::SuitText()
{
     string ret;

     // Clubs
     if ((Suit%4)==0)
     {
          ret = "Clubs";
     }
     // Spades
     else if ((Suit%4)==1)
     {
          ret = "Spades";
     }
     // Hearts
     else if ((Suit%4)==2)
     {
          ret = "Hearts";
     }
     // Diamonds (   else if ((Suit%4)==3) {  )
     else
     {
          ret = "Diamonds";
     }
     
     return ret;
}

string Card::PointsText()
{
     string ret;
     
     if (Points == 0)
        ret = "A";
     else if (Points == 10)
        ret = "J";
     else if (Points == 11)
        ret = "Q";
     else if (Points == 12)
        ret = "K";
     else
     {
        sprintf(label,"%d",Points+1);
        ret = label;
     }
     return ret;
}


bool Card::operator==(const Card &rhs)
{
     if ((Initialized == rhs.Initialized)&&(Suit == rhs.Suit)&&(Points == rhs.Points))
        return true;
     else
        return false;
}

// Take a const-reference to the right-hand side of the assignment.
// Return a non-const reference to the left-hand side.
Card& Card::operator=(const Card &rhs) {
      // Do the assignment operation!
      Initialized = rhs.Initialized;
      Suit = rhs.Suit;
      Points = rhs.Points;
      return *this;  // Return a reference to myself.
}

Card PlayerCards[5]; // Stores each player card
Card DealerCards[5]; // Stores each dealer card
Card SplitCards[11][5]; // Stores cards for later when the player hits Split

void 
drawStringBig (char *s)
{
  unsigned int i;
  for (i = 0; i < strlen (s); i++)
    glutBitmapCharacter (GLUT_BITMAP_HELVETICA_18, s[i]);
};

void drawStringReallyBig (char *s)
{
  unsigned int i;
  for (i = 0; i < strlen (s); i++)
    glutBitmapCharacter (GLUT_BITMAP_TIMES_ROMAN_24, s[i]);
}

int CalculatePlayerTotal()
{
    int total = 0;
    bool AceFound = false;
    
    // Total up "normal" cards
    for (int i=0; i < 5; i++)
    {
        if ((PlayerCards[i].Initialized) && (PlayerCards[i].Points > 0))
        {
           // Numbered card (2, 3, ... 9, 10)
           if (PlayerCards[i].Points < 10)
           {
               // Make sure to add one to PlayerCards[i].Points because of data representation
               total += PlayerCards[i].Points + 1;
           }
           // Face card (J, Q, K)
           else
           {
               total += 10;
           }
        }
    }
    // Total up Aces (first count each as one ; then optionally add ten)
    for (int i=0; i < 5; i++)
    {
        if ((PlayerCards[i].Initialized) && (PlayerCards[i].Points == 0))
        {
           total += 1;
           AceFound = true;
        }
    }
    if ((total + 10 <= 21)&&(AceFound))
       total += 10;
    
    if ((PlayerCards[4].Initialized)&&(total < 21))
    {
       total = 21;
    }
    
    return total;
}

int CalculateDealerTotal()
{
    int total = 0;
    bool AceFound = false;
    
    // Total up "normal" cards
    for (int i=0; i < 5; i++)
    {
        if ((DealerCards[i].Initialized) && (DealerCards[i].Points > 0))
        {
           // Numbered card (2, 3, ... 9, 10)
           if (DealerCards[i].Points < 10)
           {
               // Make sure to add one to DealerCards[i].Points because of data representation
               total += DealerCards[i].Points + 1;
           }
           // Face card (J, Q, K)
           else
           {
               total += 10;
           }
        }
    }
    // Total up Aces (first count each as one ; then optionally add ten)
    for (int i=0; i < 5; i++)
    {
        if ((DealerCards[i].Initialized) && (DealerCards[i].Points == 0))
        {
           total += 1;
           AceFound = true;
        }
    }
    if ((total + 10 <= 21)&&(AceFound))
       total += 10;
    
    if ((DealerCards[4].Initialized)&&(total < 21))
    {
       total = 21;
    }
    
    return total;
}

// Generate the next card at random, make sure it hasn't already been drawn
Card DealCard()
{
     Card NewCard;
     
     NewCard.Initialized = false;
     
     while (!(NewCard.Initialized))
     {
         NewCard.Initialized = true;
         // If one deck is desired, rand() % 4
         //NewCard.Suit = rand() % 4;
         // If three decks are desired, rand() % 12
         NewCard.Suit = rand() % 12;
         NewCard.Points = rand() % 13;
         
         if (!(PlayerCards[0].Initialized)&&(Cheats[1]))
            NewCard.Points = (9 + (rand() % 5))%13;
         if (!(PlayerCards[1].Initialized)&&(DealerCards[0].Initialized)&&(Cheats[1]))
         {
            if (PlayerCards[0].Points == 0)
               NewCard.Points = 9 + (rand() % 4);
            else
               NewCard.Points = 0;
            Cheats[1] = false;
         }
         if ((Cheats[3])&&!(PlayerCards[1].Initialized)&&(DealerCards[0].Initialized))
         {
            NewCard.Points = PlayerCards[0].Points;
         }
         
         for (int i=0; i < 5; i++)
         {
             if (((PlayerCards[i].Suit == NewCard.Suit)&&(PlayerCards[i].Points == NewCard.Points))||((DealerCards[i].Suit == NewCard.Suit)&&(DealerCards[i].Points == NewCard.Points)))
             {
                 NewCard.Initialized = false;
             }
         }
     }
     
     return NewCard;
}

// Basically, remove everything in UsedCardsList so all cards may be drawn again
void ShuffleDeck()
{
     for (int i=0; i<5; i++)
     {
         PlayerCards[i].Initialized = false;
         PlayerCards[i].Points = 0;
         PlayerCards[i].Suit = 0;
         DealerCards[i].Initialized = false;
         DealerCards[i].Points = 0;
         DealerCards[i].Suit = 0;
     }
}

class DebugInfo
{
public:
    void Write(char *text);
    void ForceWrite(char *text);
    void PrintToScreen();
    vector <string> Messages;
    vector <time_t> ExpiryTimes;
    ofstream OutFile;
    char dateStr [9];
    char timeStr [9];
    bool DisplayOnScreen;
    DebugInfo();
};

DebugInfo::DebugInfo()
{
    OutFile.open("log.txt");
    Write("Press 'l' for play log.  If you want help, press F1.");
    ExpiryTimes[0] = ExpiryTimes[0] + 10;
}

void DebugInfo::Write(char *text)
{
     string msg;
     _strdate( dateStr);
     _strtime( timeStr );
     msg += "[";
     msg += dateStr;
     msg += " ";
     msg += timeStr;
     msg += "] ";
     msg += text;
     msg += "\n";
     
     time_t expiry;
     if (DisplayOnScreen)
        expiry = time(NULL) + 10;
     else
        expiry = time(NULL);

     Messages.push_back(msg);
     ExpiryTimes.push_back(expiry);
     
     OutFile.write (msg.c_str(), msg.size());
}

void DebugInfo::ForceWrite(char *text)
{
     Write(text);
     ExpiryTimes.back() = time(NULL) + 10;
}

void DebugInfo::PrintToScreen()
{
    glDisable(GL_TEXTURE_2D);
    
    time_t CurrentTime = time(NULL);
    
    if ((ExpiryTimes.size())&&(DisplayOnScreen))
    {
        sprintf(label, "Debug Messages (type 'l' to disable):");
        glColor3f (1.0F, 1.0F, 0.0F);
        glRasterPos3f (-0.9F, 0.93F, -1.0F);
        drawStringReallyBig(label);
    }
    
    for (int i=0; i < Messages.size(); i++)
    {
        if (CurrentTime<ExpiryTimes[i])
        {
            float ColorValue;
            ColorValue = 1.0F - (((float)(ExpiryTimes[i]-CurrentTime))/8.0F);
            sprintf(label, "%s", Messages[i].c_str());
            glColor3f(ColorValue, ColorValue, ColorValue);
            //glColor3f ( (((float)(i))/8.0F), (((float)(i))/8.0F), (((float)(i))/8.0F));
            glRasterPos3f (-0.9F, (0.85F - ((float)i)*0.1F ), -1.0F);
            drawStringBig(label);
        }
        else
        {
            Messages.erase(Messages.begin() + i);
            ExpiryTimes.erase(ExpiryTimes.begin() +i);
        }
    } 
    glEnable(GL_TEXTURE_2D);
}

DebugInfo Info;

/*
void ProcessCheats()
{
     // Show Dealer's Card Points
     if (Cheats[1])
     {
        //static char label[100];
        //glDisable(GL_TEXTURE_2D);
        sprintf(label, "%d",CalculateDealerTotal());
        glColor3f (0.9F, 0.9F, 0.9F);
        glRasterPos3f (-0.4F, 0.9F, -1.0F);
        drawStringReallyBig(label);
        //glEnable(GL_TEXTURE_2D);
     }
     // Deal player a BlackJack
     //if (Cheats[2])
}
*/

void ShowTopScore()
{
    sprintf(label, "Top Score: $%d",HighScore);
    Info.ForceWrite(label);
}

void DrawTexture(string load_me, float x1, float y1, float x2, float y2)
{
	GLuint tex_ID;
	bool found;
	/*
 	int TexturePosition;
    for (TexturePosition = 0; TexturePosition < TextureStrings.size(); TexturePosition++)
    {
       if (TextureStrings.at(TexturePosition)==load_me)
       {
          found = true;
          break;
       }
    }
    */
    
    iter = Textures.find(load_me);
    
    //found = (iter != Textures.end());

    //if (!found)
    if (iter != Textures.end())
    {
        //TexturePosition++;
    	
		//	load the texture
		tex_ID = SOIL_load_OGL_texture(
				load_me.c_str(),
				SOIL_LOAD_AUTO,
				SOIL_CREATE_NEW_ID,
				SOIL_FLAG_POWER_OF_TWO
				| SOIL_FLAG_MIPMAPS
				| SOIL_FLAG_MULTIPLY_ALPHA
				//| SOIL_FLAG_COMPRESS_TO_DXT
				| SOIL_FLAG_DDS_LOAD_DIRECT
				//| SOIL_FLAG_NTSC_SAFE_RGB
				//| SOIL_FLAG_CoCg_Y
				//| SOIL_FLAG_TEXTURE_RECTANGLE
				);
				
        //TextureStrings.push_back(load_me);
        //TextureIDs.push_back(tex_ID);
        Textures.insert( make_pair(load_me, tex_ID) );
    }
    // We have loaded the texture already
    else
    {
		tex_ID = SOIL_load_OGL_texture(
				load_me.c_str(),
				SOIL_LOAD_AUTO,
				//TextureIDs.at(TexturePosition),
                (*iter).second,
				SOIL_FLAG_POWER_OF_TWO
				| SOIL_FLAG_MIPMAPS
				| SOIL_FLAG_MULTIPLY_ALPHA
				//| SOIL_FLAG_COMPRESS_TO_DXT
				| SOIL_FLAG_DDS_LOAD_DIRECT
				//| SOIL_FLAG_NTSC_SAFE_RGB
				//| SOIL_FLAG_CoCg_Y
				//| SOIL_FLAG_TEXTURE_RECTANGLE
				);
    }
    
    const float ref_mag = 0.1f;
    const float tex_u_max = 1.0f;
    const float tex_v_max = 1.0f;

	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    glBegin(GL_QUADS);
		glNormal3f( -ref_mag, -ref_mag, 1.0f );
        glTexCoord2f( 0.0f, tex_v_max );
        glVertex3f( x2, y1, -0.1f );

        glNormal3f( ref_mag, -ref_mag, 1.0f );
        glTexCoord2f( tex_u_max, tex_v_max );
        glVertex3f( x1, y1, -0.1f );

        glNormal3f( ref_mag, ref_mag, 1.0f );
        glTexCoord2f( tex_u_max, 0.0f );
        glVertex3f( x1, y2, -0.1f );

        glNormal3f( -ref_mag, ref_mag, 1.0f );
        glTexCoord2f( 0.0f, 0.0f );
        glVertex3f( x2, y2, -0.1f );
    glEnd();    
}

void HandleHits()
{
    // See if the player has hit (or double downed)
    if ((HitPressed) && !(StayPressed))
    {
       HitPressed = false;
       if (PlayerTotal<21)
       {
           int position;
           for (position = 0; position < 5; position++)
           {
               if (!PlayerCards[position].Initialized)
                  break;
           }
           PlayerCards[position] = DealCard();
           
           sprintf(label,"Player hit and got a %s of %s", PlayerCards[position].PointsText().c_str(), PlayerCards[position].SuitText().c_str());
           Info.Write(label);
           
           PlayerTotal = CalculatePlayerTotal();
           
           if (PlayerTotal>=21)
              StayPressed = true;
       }
       sndPlaySound("lib\\hit.wav", SND_ASYNC);
    }
}

void HandleNewHand()
{
    if (((PlayerTotal>DealerTotal)||(DealerTotal>21))&&(PlayerTotal<=21))
    {
       Money = Money + CurrentBet;
    }
    else if ((PlayerTotal<DealerTotal)||(PlayerTotal>21))
    {
      // Make sure that if the dealer won, the player wasn't insured
      if (!InsurancePressed)
         Money = Money - CurrentBet;
    }
    
    // Check to see if the player (foolishly) bought insurance...
    if ((InsurancePressed)&&!((DealerTotal==21)&&!(DealerCards[2].Initialized)))
    Money = Money - ((CurrentBet + 1) / 2);
    
    EndMessage = (char)(((int)'a')+rand() % 4);
    StayPressed = false;
    HitPressed = false;
    ShowDealerCards = false;
    NewPressed = false;
    DoubleDownPressed = false;
    SplitPressed = false;
    InsurancePressed = false;
    CannotPlaySound = false;
    sndPlaySound("lib\\newhand.wav", SND_ASYNC);
}

// HandleSplits needs to serve two purposes: to properly manage each player's
// split and to also go back through each hand to tell the player if win/lose.
// To do this, HandleSplits must also catch "Next Hand" when there are pending
// splits.
/*
void HandleSplits()
{
    // Determine how many splits we have stored (and what position to use)
    int counter;
    //counter = SplitCounter;
    
    
    for (counter = 0; counter < 11; counter++)
    {
       if (!SplitCards[counter][0].Initialized)
          break;
    }
    
    if ((counter==0)&&(!SplitPressed))
    {
       SplitCounter = 0;
       NumberOfSplits = 0;
       return;
    }
    
    // test
    counter = counter - 1;
    
    if (counter == NumberOfSplits)
    {
       SplitMode = 2;
    }
    
    // Check to see if the player pressed the split button/key
    if ((SplitPressed)&&(PlayerCards[0].Points==PlayerCards[1].Points))
    {
       Info.Write("if ((SplitPressed)&&(PlayerCards[0].Points==PlayerCards[1].Points))");
       NumberOfSplits++;
       sprintf(label,"NumberOfSplits++; (%d)",NumberOfSplits);
       Info.Write(label);
       //SplitCounter++;
       
       SplitCards[counter+1][0] = PlayerCards[1];
       sprintf(label,"SplitCards[%d][0] = PlayerCards[1];",counter + 1);
       Info.Write(label);
       
       //PlayerCards[1].Initialized = false;
       PlayerCards[1] = DealCard();
       
       sprintf(label,"PlayerCards[1] = DealCard(); (%s of %s)",PlayerCards[1].PointsText().c_str(),PlayerCards[1].SuitText().c_str());
       Info.Write(label);
       //SplitPressed = false;
       SplitMode = 1;
    }
    
    // / *
    //if ((HitPressed)&&(NumberOfSplits))
    //{
    //   Info.Write("if ((HitPressed)&&(NumberOfSplits)): HandleHits();");
    //   HandleHits();
    //   //SplitCards[counter] = PlayerCards;
    //}
    // * /
    
    // fix this conditional
    //if ((StayPressed)&&(SplitCounter<NumberOfSplits)&&!(SplitCards[counter][0]==PlayerCards[0]))
    if ((StayPressed)&&(SplitMode==1))
    {
        Info.Write("if ((StayPressed)&&(SplitCounter<NumberOfSplits)&&(SplitCounter<NumberOfSplits))");
        SplitCards[counter][0] = PlayerCards[0];
        SplitCards[counter][1] = PlayerCards[1];
        SplitCards[counter][2] = PlayerCards[2];
        SplitCards[counter][3] = PlayerCards[3];
        SplitCards[counter][4] = PlayerCards[4];
        sprintf(label,"SplitCards[counter][0 thru 4] = PlayerCards[0 thru 4];",counter + 1);
        Info.Write(label);
        SplitCounter++;
        counter++;
        sprintf(label,"SplitCounter++; (%d)",SplitCounter);
        Info.Write(label);
        StayPressed = false;
        if (counter == NumberOfSplits - 1)
          SplitMode = 2;
    }
    
    // fix this conditional too
    //if ((NewPressed)&&(SplitCards[counter][0].Initialized)&&(SplitCounter==NumberOfSplits)/ *&&(NumberOfSplits>=0)* /)
    if ((NewPressed)&&(NumberOfSplits>=0))
    {          
       if ((SplitCards[counter][0].Initialized)&&(SplitMode==2))
       {
           Info.Write("if (NewPressed) { if ((NumberOfSplits>0)&&(SplitCards[counter][0].Initialized)&&(SplitCounter==NumberOfSplits))");
           for (int i=0; i < 5; i++)
           {
               PlayerCards[i] = SplitCards[counter][i];
               sprintf(label,"PlayerCards[%d] = SplitCards[%d][%d];",i,counter + 1,i);
               Info.Write(label);
               SplitCards[counter][i].Initialized = false;
               sprintf(label,"SplitCards[%d][%d].Initialized = false;",counter + 1,i);
               Info.Write(label);
           }
           //NumberOfSplits--;
           SplitCounter--;
           //counter--;
           //SplitCounter++;
           //sprintf(label,"NumberOfSplits--; (%d)",NumberOfSplits);
           //Info.Write(label);
           sprintf(label,"SplitCounter--; (%d)",SplitCounter);
           Info.Write(label);
       }
       else if (SplitMode==1)
       {
           if (!PlayerCards[1].Initialized)
           {
               Info.Write("if (!PlayerCards[1].Initialized)");
               PlayerCards[1] = DealCard();
               sprintf(label,"PlayerCards[1] = DealCard(); (%s of %s)",PlayerCards[1].PointsText().c_str(),PlayerCards[1].SuitText().c_str());
               Info.Write(label);
               SplitCards[counter][1] = PlayerCards[1];
               sprintf(label,"SplitCards[%d][1] = PlayerCards[1];",counter);
               Info.Write(label);
           }
       }

    }
    
    SplitPressed = false;
    //Info.Write("SplitPressed = false");
}
// */

void resize(int w, int h)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glViewport(0, 0, w, h);
	gluOrtho2D(0.0, 1.0, 0.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}



int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    WNDCLASSEX wcex;
    HWND hwnd;
    HDC hDC;
    HGLRC hRC;
    MSG msg;
    BOOL bQuit = FALSE;
    string load_me;
    
    // register window class
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "BlackJack";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
        return 0;

    // create main window
    hwnd = CreateWindowEx(0,
                          "BlackJack",
                          "BlackJack by Sam",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          800,
                          600,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    ShowWindow(hwnd, nCmdShow);

    // enable OpenGL for the window
    EnableOpenGL(hwnd, &hDC, &hRC);

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );

    //glReshapeFunc(resize);

    ifstream ScoreFileIn, MoneyFileIn;
    ScoreFileIn.open("highscore.txt");
    ScoreFileIn >> HighScore;
    ScoreFileIn.close();
    MoneyFileIn.open("money.txt");
    MoneyFileIn >> Money;
    MoneyFileIn.close();
    
    if (Money < 100)
    {
       sprintf(label,"Saved Money ($%d) is less than $100, you now have $100",Money);
       Info.ForceWrite(label);
       Money = 100;
    }
    ShowTopScore();
    
    // program main loop

    while (!bQuit)
    {
        srand((unsigned)time(0));  
        
        if (Money>HighScore)
        {
           sprintf(label,"!!!!!New Top Score!!!!! $%d (was $%d)", Money, HighScore);
           Info.DisplayOnScreen = CurrentLine;
           ofstream ScoreFileOut;
           ScoreFileOut.open("highscore.txt");
           ScoreFileOut << Money;
           ScoreFileOut.close();
           HighScore = Money;
        }
        
        //HandleSplits();
        
        // See if dealer caught the player with an uninsured BlackJack
        if (((DealerTotal==21)&&!(DealerCards[2].Initialized))&&((HitPressed)||(DoubleDownPressed)||(SplitPressed)))
        {
           StayPressed = true;
           HitPressed = false;
           DoubleDownPressed = false;
           SplitPressed = false;
        }
        if (PlayerTotal==21)
           StayPressed = true;
           
        if (NewPressed)
        {
             HandleNewHand();
             
             if (/*(!NumberOfSplits)&&*/(!SplitCounter)&&(!(SplitCards[0][0].Initialized)&&!(SplitCards[1][0].Initialized)&&!(SplitCards[2][0].Initialized)&&!(SplitCards[3][0].Initialized)&&!(SplitCards[4][0].Initialized)&&!(SplitCards[5][0].Initialized)&&!(SplitCards[6][0].Initialized)&&!(SplitCards[7][0].Initialized)&&!(SplitCards[8][0].Initialized)&&!(SplitCards[9][0].Initialized)&&!(SplitCards[10][0].Initialized)))
             {
                 CurrentBet = Wager;
                           
                 ShuffleDeck();
                 PlayerCards[0] = DealCard();
                 sprintf(label,"PlayerCards[0] = DealCard(); (%s of %s)",PlayerCards[0].PointsText().c_str(),PlayerCards[0].SuitText().c_str());
                 Info.Write(label);
                 DealerCards[0] = DealCard();
                 sprintf(label,"DealerCards[1] = DealCard(); (%s of %s)",DealerCards[0].PointsText().c_str(),DealerCards[0].SuitText().c_str());
                 Info.Write(label);
                 PlayerCards[1] = DealCard();
                 sprintf(label,"PlayerCards[0] = DealCard(); (%s of %s)",PlayerCards[1].PointsText().c_str(),PlayerCards[1].SuitText().c_str());
                 Info.Write(label);
                 DealerCards[1] = DealCard();
                 sprintf(label,"DealerCards[1] = DealCard(); (%s of %s)",DealerCards[1].PointsText().c_str(),DealerCards[1].SuitText().c_str());
                 Info.Write(label);
                 PlayerTotal = CalculatePlayerTotal();
                 DealerTotal = CalculateDealerTotal();
             }
        }

        
        // Check to see if double down is pressed
        if (DoubleDownPressed)
        {
           // After we handle the hit we need to set DoubleDownPressed = false;
           HitPressed = true;
           CurrentBet = CurrentBet * 2;
        }
        // Process hits
        HandleHits();
        
        // Finish handling a double down (if applicable)
        if (DoubleDownPressed)
        {
           DoubleDownPressed = false;
           StayPressed = true;
           sndPlaySound("lib\\doubledown.wav", SND_ASYNC);
        }
        if ((StayPressed)&&!(SplitCounter))
        {
           ShowDealerCards = true;
           // Give the dealer a hit if he's <=16 and the player isn't bust and the player doesn't have blackjack
           if (((DealerTotal <= 16)&&(PlayerTotal<=21)&&!((PlayerTotal==21)&&!(PlayerCards[2].Initialized)))||((Cheats[2])&&(DealerTotal<21)))
           {
               int position;
               for (position = 0; position < 5; position++)
               {
                   if (!DealerCards[position].Initialized)
                      break;
               }
               DealerCards[position] = DealCard();
               DealerTotal = CalculateDealerTotal();
               //Sleep(1000);
           }
           
        }     
        
        // check for messages
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // handle or dispatch messages
            if (msg.message == WM_QUIT)
            {
                bQuit = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            glClearColor(0.01f, 0.741f, 0.075f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            glDisable(GL_TEXTURE_2D);

            if (DealerCards[0].Initialized)
            {
                sprintf(label, "%d",PlayerTotal);
                glColor3f (0.9F, 0.86F, 0.9F);
                glRasterPos3f (-0.4F, -0.92F, -1.0F);
                drawStringReallyBig(label);
                if ((ShowDealerCards)||(Cheats[0]))
                {
                    sprintf(label, "%d",DealerTotal);
                    glColor3f (0.9F, 0.9F, 0.9F);
                    glRasterPos3f (-0.4F, 0.9F, -1.0F);
                    drawStringReallyBig(label);
                }
            }         
            
            glColor3f(1.0F, 1.0F, 1.0F);
            sprintf(label, "Cash: $%d",Money);
            load_me.assign(label);
            sprintf(label,load_me.c_str());
            glRasterPos3f (0.6F, 0.8F, -1.0F);
            drawStringReallyBig(label);
            
            sprintf(label, "Wager: $%d",Wager);
            load_me.assign(label);
            sprintf(label,load_me.c_str());
            glRasterPos3f (0.6F, 0.4F, -1.0F);
            drawStringReallyBig(label);                
            
            glEnable(GL_TEXTURE_2D);
            
            // Table Decorations
            load_me = "images\\blackjack.png";
            DrawTexture(load_me, 0.4f, -0.2f, -0.8f, 0.2f);
            
            //Menu buttons
            if ((!StayPressed)&&(DealerCards[0].Initialized))
            {
                load_me = "images\\hit.png";
                DrawTexture(load_me, 1.0f, -0.35f, 0.55f, -0.15f);     
                load_me = "images\\stay.png";
                DrawTexture(load_me, 1.0f, -0.55f, 0.55f, -0.35f); 
                
                // Check to see if the player can double down
                if (!(DoubleDownPressed)&&!(PlayerCards[2].Initialized))
                {
                   load_me = "images\\doubledown.png";
                   DrawTexture(load_me,0.55f, -0.35f, 0.1f, -0.15f);
                }
                // Check to see if the player can insure (dealer has ace showing, player only at 2 cards)
                if (!(InsurancePressed)&&!(PlayerCards[2].Initialized)&&!(DealerCards[0].Points))
                {
                   load_me = "images\\insurance.png";
                   DrawTexture(load_me,0.55f, -0.55f, 0.1f, -0.35f);
                }
                /*
                if (!(SplitPressed)&&(PlayerCards[0].Points==PlayerCards[1].Points))
                {
                   load_me = "images\\split.png";
                   DrawTexture(load_me, 0.55f, -0.75f, 0.1f, -0.55f);
                }
                */
            }
                          
            load_me = "images\\quit.png";
            DrawTexture(load_me, 1.0f, -1.0f, 0.55f, -0.8f);
            
            
            // Dealer label
            load_me = "images\\dealer.png";
            DrawTexture(load_me, -0.40f, 0.8f, -0.90f, 1.0f);
            
            // Player label
            load_me = "images\\you.png";
            DrawTexture(load_me, -0.40f, -1.0f, -0.90f, -0.8f);
            
            // Player Card 1
            if (PlayerCards[0].Initialized)
            {
                sprintf(label,"images\\%d%c.png", ((52-(PlayerCards[0].Points * 4))+(PlayerCards[0].Suit%4))%52 + 1, 'a'+((char) (PlayerCards[0].Suit % 3)));
                load_me.assign(label);
                DrawTexture(load_me, -0.65f, -0.817f, -0.90f, -0.283f);
            }

            // Player Card 2
            if (PlayerCards[1].Initialized)
            {
                sprintf(label,"images\\%d%c.png", ((52-(PlayerCards[1].Points * 4))+(PlayerCards[1].Suit%4))%52 + 1, 'a'+((char) (PlayerCards[1].Suit % 3)));
                load_me.assign(label);
                DrawTexture(load_me, -0.40f, -0.817f, -0.65f, -0.283f);
            }
            // Player Card 3
            if (PlayerCards[2].Initialized)
            {
                sprintf(label,"images\\%d%c.png", ((52-(PlayerCards[2].Points * 4))+(PlayerCards[2].Suit%4))%52 + 1, 'a'+((char) (PlayerCards[2].Suit % 3)));
                load_me.assign(label);
                DrawTexture(load_me, -0.15f, -0.817f, -0.40f, -0.283f);
            }
            // Player Card 4
            if (PlayerCards[3].Initialized)
            {
                sprintf(label,"images\\%d%c.png", ((52-(PlayerCards[3].Points * 4))+(PlayerCards[3].Suit%4))%52 + 1, 'a'+((char) (PlayerCards[3].Suit % 3)));
                load_me.assign(label);
                DrawTexture(load_me, 0.10f, -0.817f, -0.15f, -0.283f);
            }
            // Player Card 5
            if (PlayerCards[4].Initialized)
            {
                sprintf(label,"images\\%d%c.png", ((52-(PlayerCards[4].Points * 4))+(PlayerCards[4].Suit%4))%52 + 1, 'a'+((char) (PlayerCards[4].Suit % 3)));
                load_me.assign(label);
                DrawTexture(load_me, 0.35f, -0.817f, 0.10f, -0.283f);
            }
            
            // Dealer Card 1
            if (DealerCards[0].Initialized)
            {
                sprintf(label,"images\\%d%c.png", ((52-(DealerCards[0].Points * 4))+(DealerCards[0].Suit%4))%52 + 1, 'a'+((char) (DealerCards[0].Suit % 3)));
                load_me.assign(label);
                DrawTexture(load_me, -0.65f, 0.283f, -0.90f, 0.817f);
            }
            // Dealer Card 2 (hide this one from the player)
            if (DealerCards[1].Initialized)
            {
                if (ShowDealerCards)
                {
                    sprintf(label,"images\\%d%c.png", ((52-(DealerCards[1].Points * 4))+(DealerCards[1].Suit%4))%52 + 1, 'a'+((char) (DealerCards[1].Suit % 3)));
                    load_me.assign(label);
                }
                else
                {
                    sprintf(label,"images\\back%c.png",EndMessage);
                    load_me.assign(label);
                }
                
                DrawTexture(load_me, -0.40f, 0.283f, -0.65f, 0.817f);
                glDisable(GL_TEXTURE_2D);
                //glRasterPos3f(0.2f,0.2f,-1.0f);
                //drawStringBig((char*)load_me.c_str());
                glEnable(GL_TEXTURE_2D);
            }
            
            // Dealer Card 3
            if (DealerCards[2].Initialized)
            {
                sprintf(label,"images\\%d%c.png", ((52-(DealerCards[2].Points * 4))+(DealerCards[2].Suit%4))%52 + 1, 'a'+((char) (DealerCards[2].Suit % 3)));
                load_me.assign(label);
                DrawTexture(load_me, -0.15f, 0.283f, -0.40f, 0.817f);
            }
            // Dealer Card 4
            if (DealerCards[3].Initialized)
            {
                sprintf(label,"images\\%d%c.png", ((52-(DealerCards[3].Points * 4))+(DealerCards[3].Suit%4))%52 + 1, 'a'+((char) (DealerCards[3].Suit % 3)));
                load_me.assign(label);
                DrawTexture(load_me, 0.10f, 0.283f, -0.15f, 0.817f);
            }
            // Dealer Card 5
            if (DealerCards[4].Initialized)
            {
                sprintf(label,"images\\%d%c.png", ((52-(DealerCards[4].Points * 4))+(DealerCards[4].Suit%4))%52 + 1, 'a'+((char) (DealerCards[4].Suit % 3)));
                load_me.assign(label);
                DrawTexture(load_me, 0.35f, 0.283f, 0.10f, 0.817f);
            }
            
            // Tell the player if he/she is bust, winner, loser, etc

            if ((StayPressed)||!(DealerCards[0].Initialized))
            {
               // Player pressed stay and isn't bust (and the dealer has finished {possibly due to player blackjack})..
                if (((PlayerTotal<=21)&&((DealerTotal>16)||(PlayerTotal==21)))&&(!SplitCounter))
                {
                    if ((PlayerTotal>DealerTotal)||(DealerTotal>21))
                    {
                        // See if it's BlackJack and display BlackJack win graphic
                        if ((!PlayerCards[2].Initialized)&&(PlayerTotal==21))
                        {
                            sprintf(label,"images\\%d%c.png", ((52-(PlayerCards[0].Points * 4))+(PlayerCards[0].Suit%4))%52 + 1, 'a'+((char) (PlayerCards[0].Suit % 3)));
                            load_me.assign(label);
                            DrawTexture(load_me, -0.05f, -0.40f, -0.40f, 0.40f);
                            sprintf(label,"images\\%d%c.png", ((52-(PlayerCards[1].Points * 4))+(PlayerCards[1].Suit%4))%52 + 1, 'a'+((char) (PlayerCards[1].Suit % 3)));
                            load_me.assign(label);
                            DrawTexture(load_me, 0.40f, -0.40f, 0.05f, 0.40f);
                            sprintf(label,"images\\confetti.png");
                            load_me.assign(label);
                            DrawTexture(load_me, 1.0f, 1.0f, -1.0f, -1.0f);
                            sprintf(label,"images\\blackjackwin.png");
                            load_me.assign(label);
                            DrawTexture(load_me, 0.7f, 0.45f, -0.7f, 0.8f);
                            if (!CannotPlaySound)
                            {
                               sndPlaySound("lib\\blackjack.wav", SND_ASYNC);
                               CannotPlaySound = true;
                            }
                        }
                        // normal win graphic
                        else
                        {
                            load_me = "images\\win";
                            load_me.append(&EndMessage);
                            load_me.append(".png");
                            DrawTexture(load_me, 0.5f, -0.7f, -0.8f, 0.7f);  
                            if (!CannotPlaySound)
                            {
                               sndPlaySound("lib\\yay.wav", SND_ASYNC);
                               CannotPlaySound = true;
                            }
                        }
                    }
                    else if ((PlayerTotal<DealerTotal)&&!((InsurancePressed==true) && (DealerTotal==21) && !(DealerCards[2].Initialized)))
                    {
                        load_me = "images\\lose";
                        load_me.append(&EndMessage);
                        load_me.append(".png");
                        DrawTexture(load_me, 0.5f, -0.7f, -0.8f, 0.7f);
                        if (!CannotPlaySound)
                        {
                           sndPlaySound("lib\\groan.wav", SND_ASYNC);
                           CannotPlaySound = true;
                        }
                    }
                    else if (((PlayerTotal==DealerTotal)&&(DealerCards[0].Initialized))||((InsurancePressed==true) && (DealerTotal==21) && !(DealerCards[2].Initialized)))
                    {
                        load_me = "images\\push.png";
                        DrawTexture(load_me, 0.5f, -0.7f, -0.8f, 0.7f);
                        if (!CannotPlaySound)
                        {
                           sndPlaySound("lib\\push.wav", SND_ASYNC);
                           CannotPlaySound = true;
                        }
                    }
                }
                // Check to see if player is bust
                else if (PlayerTotal>21)
                {
                    if (!SplitCounter)
                       ShowDealerCards = true;
                    load_me = "images\\bust";
                    load_me.append(&EndMessage);
                    load_me.append(".png");
                    DrawTexture(load_me, 0.5f, -0.7f, -0.8f, 0.7f);    
                }
                // Show New Game button
                load_me = "images\\newgame.png";
                DrawTexture(load_me, 1.0f, -0.8f, 0.55f, -0.6f);
                // Wager buttons
                load_me = "images\\plus1.png";
                DrawTexture(load_me, 0.7f, 0.1f, 0.55f, 0.3f);
                load_me = "images\\plus5.png";
                DrawTexture(load_me, 0.85f, 0.1f, 0.7f, 0.3f);
                load_me = "images\\plus25.png";
                DrawTexture(load_me, 1.0f, 0.1f, 0.85f, 0.3f);
                load_me = "images\\clearwager.png";
                DrawTexture(load_me, 1.0f, -0.1f, 0.55f, 0.1f);  
                
            }
            
            
            {
				/*	check for errors	*/
				GLenum err_code = glGetError();
				while( GL_NO_ERROR != err_code )
				{
					printf( "OpenGL Error @ %s: %i", "drawing loop", err_code );
					err_code = glGetError();
				}
			}
			
            // Print out debug messages
            Info.PrintToScreen();

            SwapBuffers(hDC);
            
            Sleep (1);
        }

    }

    // shutdown OpenGL
    DisableOpenGL(hwnd, hDC, hRC);

    // destroy the window explicitly
    DestroyWindow(hwnd);

    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    bool DisplayLog = Info.DisplayOnScreen;
    int i;
    ofstream OutFile;
    
    switch (uMsg)
    {
        case WM_CLOSE:
             OutFile.open("money.txt");
             OutFile << Money;
             OutFile.close();
            PostQuitMessage(0);
        break;

        case WM_DESTROY:
            return 0;

        case WM_LBUTTONUP:
             MouseX = LOWORD(lParam);
             MouseY = HIWORD(lParam);
             MouseXf = (float)MouseX / 800.0f;
             MouseYf = (float)MouseY / 600.0f;
             
             // Test for Clear Wager button
             if (((MouseXf >= 0.75f) && (MouseYf >= 0.42f)) && ((MouseXf <= 1.0f) && (MouseYf <= 0.5f)))
             {
                Wager = 0;
             }
             // Test for wager increment buttons
             if (((MouseXf >= 0.75f) && (MouseYf >= 0.32f)) && ((MouseXf <= 0.83f) && (MouseYf <= 0.42f)))
             {
                Wager += 1;
             }
             if (((MouseXf >= 0.84f) && (MouseYf >= 0.32f)) && ((MouseXf <= 0.9f) && (MouseYf <= 0.42f)))
             {
                Wager += 5;
             }
             if (((MouseXf >= 0.91f) && (MouseYf >= 0.32f)) && ((MouseXf <= 1.0f) && (MouseYf <= 0.42f)))
             {
                Wager += 25;
             }
             
             // Test for Hit button
             if (((MouseXf >= 0.75f) && (MouseYf >= 0.55f)) && ((MouseXf <= 1.0f) && (MouseYf <= 0.63f)))
             {
                HitPressed = true;
             }
             // Test for Stay button
             if (((MouseXf >= 0.75f) && (MouseYf >= 0.63f)) && ((MouseXf <= 1.0f) && (MouseYf <= 0.73f)))
             {
                StayPressed = true;
             }
             // Test for Double Down button
             if (((MouseXf >= 0.5f) && (MouseYf >= 0.55f)) && ((MouseXf <= 0.75f) && (MouseYf <= 0.63f)))
             {
                if (!(PlayerCards[2].Initialized))
                   DoubleDownPressed = true;
             }
             // Test for Insurance button
             if (((MouseXf >= 0.5f) && (MouseYf >= 0.63f)) && ((MouseXf <= 0.75f) && (MouseYf <= 0.73f)))
             {
                if (!(PlayerCards[2].Initialized)&&!(DealerCards[0].Points))
                {
                   InsurancePressed = true;
                   sndPlaySound("lib\\insurance.wav", SND_ASYNC);
                }
             }
             // Test for Split button
             if (((MouseXf >= 0.5f) && (MouseYf >= 0.71f)) && ((MouseXf <= 0.75f) && (MouseYf <= 0.83f)))
             {
                if (PlayerCards[0].Points==PlayerCards[1].Points)
                   SplitPressed = true;
             }
             // Test for New Game button
             if (((MouseXf >= 0.75f) && (MouseYf >= 0.75f)) && ((MouseXf <= 1.0f) && (MouseYf <= 0.85f)))
             {
                NewPressed = true;
                Info.Write("Player clicked Next Hand");
             }
             // Test for Quit button
             if (((MouseXf >= 0.75f) && (MouseYf >= 0.85f)) && ((MouseXf <= 1.0f) && (MouseYf <= 1.0f)))
             {
                 OutFile.open("money.txt");
                 OutFile << Money;
                 OutFile.close();
                QuitPressed = true;
                PostQuitMessage(0);
             }
             
        break;
        
        case WM_HELP:
             if (Info.Messages.size()>10)
                break;
                
             if (!Info.DisplayOnScreen)
             {
                Info.DisplayOnScreen = true;
             }
             CurrentLine = Info.Messages.size();
             Info.Write("                   ---Help---");
             Info.Write("If you would like to know the rules of BlackJack, please");
             Info.Write("visit http://www.blackjackinfo.com/blackjack-rules.php");
             Info.Write("Basically, with each hand you're dealt, you want to get a");
             Info.Write("hand that beats the dealer's hand and doesn't bust (go over");
             Info.Write("21 points in value).  Face cards count for 10 points each");
             Info.Write("and Aces count for 1 or 11 points (1 if an 11 would bust");
             Info.Write("your hand).  You will see buttons appear on the right side");
             Info.Write("of the game window, which allow you to start a new hand,");
             Info.Write("hit (be dealt another card), stay (stop dealing cards to");
             Info.Write("you so you can see if you beat the dealer), double down");
             Info.Write("(double your wager and receive only one card), or buy");
             Info.Write("insurance (If the dealer is showing an ace, for a portion");
             Info.Write("of your wager you will break even if the dealer was dealt");
             Info.Write("with a BlackJack).  A BlackJack (being dealt with 21 points)");
             Info.Write("automatically wins the game for whoever holds one.");
             Info.Write("If you want to cheat, there are cheats built in to this");
             Info.Write("game, but I'm not going to tell you what they are, you have");
             Info.Write("to look at the source code yourself to find them (it isn't");
             Info.Write("very hard).  There are some useful keyboard shortcuts:");
             Info.Write("pressing 'n' will start a new hand, 'h' will deal you a");
             Info.Write("card (Hit), 's' will stay, 'd' will double down, 'i' will");
             Info.Write("buy insurance, 'l' will display the play log on screen,");
             Info.Write("'t' will briefly display the top score, and the F1 key will");
             Info.Write("show this help text.  Again, the BlackJack rules web site");
             Info.Write("is at http://www.blackjackinfo.com/blackjack-rules.php");
             Info.Write("and you can get the latest copy of this program at");
             Info.Write("http://pollux.csustan.edu/~sbosshar/BlackJack.zip");
             Info.DisplayOnScreen = DisplayLog;
             for (i=CurrentLine; i<CurrentLine+28; i++)
                 Info.ExpiryTimes[i] = Info.ExpiryTimes[i] + (i * 4);
        break;
        
        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_ESCAPE:
                    PostQuitMessage(0);
                break;
                // find key codes @ http://delphi.about.com/od/objectpascalide/l/blvkc.htm
                // h key
                case 0x48:
                     HitPressed = true;
                     Info.Write("Player pressed Hit");
                break;
                // s key
                case 0x53:
                     StayPressed = true;
                     Info.Write("Player pressed Stay");
                break;
                // n key
                case 0x4E:
                     NewPressed = true;
                     Info.Write("Player pressed Next Hand");
                break;
                // c key
                case 0x43:
                     Wager = 0;
                break;
                // 1 (bet $1) key
                case 0x31:
                     Wager += 1;
                break;
                // 2 (bet $5) key
                case 0x32:
                     Wager += 5;
                break;
                // 3 (bet $25) key
                case 0x33:
                     Wager += 25;
                break;
                // d (Double Down)
                case 0x44:
                     if (!(PlayerCards[2].Initialized))
                        DoubleDownPressed = true;
                     Info.Write("Player pressed Double Down");
                break;
                // i (Insurance)
                case 0x49:
                     if (!(PlayerCards[2].Initialized)&&!(DealerCards[0].Points))
                        InsurancePressed = true;
                     Info.Write("Player pressed Insurance");
                     sndPlaySound("lib\\insurance.wav", SND_ASYNC);
                break;
                // p (Split)
                case 0x50:
                     if (PlayerCards[0].Points==PlayerCards[1].Points)
                        SplitPressed = true;
                     Info.Write("Player pressed Split");
                break;
                // 50 4f 4e 4d 4c 4b 4a 49 48 47 l key
                case 0x4C:
                     Info.DisplayOnScreen = !Info.DisplayOnScreen;
                     sprintf(label,"Display Play Log set to %d",Info.DisplayOnScreen);
                     Info.Write(label);
                break;
                // t key (Top Score)
                case 0x54:
                     ShowTopScore();
                break;
                // q key
                case 0x51:
                     QuitPressed = true;
                     Info.Write("Player pressed Quit");
                     OutFile.open("money.txt");
                     OutFile << Money;
                     OutFile.close();
                     PostQuitMessage(0);
                break;
                
                // Z (display dealer points)
                case 0x5A:
                     Cheats[0] = !Cheats[0];
                     sprintf(label, "(Spy Cheat) Show Dealer Points set to %d", Cheats[0]);
                     Info.ForceWrite(label);
                break;
                // J (Attempt to deal player a BlackJack)
                case 0x4A:
                     Cheats[1] = !Cheats[1];
                     sprintf(label, "(BlackJack Cheat) Give BlackJack next hand");
                     Info.ForceWrite(label);
                break;
                // B (Make the dealer hit on 20 and lower)
                case 0x42:
                     Cheats[2] = !Cheats[2];
                     sprintf(label, "(Dealer Bust Cheat) Dealer hits on <=20 set to %d", Cheats[2]);
                     Info.ForceWrite(label);
                break;
                // Y (Give player a split)
                case 0x59:
                     Cheats[3] = !Cheats[3];
                     sprintf(label, "(Split Cheat) Give Player Splits set to %d", Cheats[3]);
                     Info.ForceWrite(label);
                break;
                // M (Add wager to Money)
                case 0x4D:
                     sprintf(label, "(Money Cheat) Add current wager to money ($%d + $%d = $%d)", Wager, Money, Wager+Money);
                     Money += Wager;
                     Info.ForceWrite(label);
                break;
            }
        }
        break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC)
{
    PIXELFORMATDESCRIPTOR pfd;

    int iFormat;

    /* get the device context (DC) */
    *hDC = GetDC(hwnd);

    /* set the pixel format for the DC */
    ZeroMemory(&pfd, sizeof(pfd));

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW |
                  PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    iFormat = ChoosePixelFormat(*hDC, &pfd);

    SetPixelFormat(*hDC, iFormat, &pfd);

    /* create and enable the render context (RC) */
    *hRC = wglCreateContext(*hDC);

    wglMakeCurrent(*hDC, *hRC);
}

void DisableOpenGL (HWND hwnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd, hDC);
}

