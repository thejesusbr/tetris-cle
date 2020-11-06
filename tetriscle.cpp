#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <ncurses.h>
#include <cmath>
#include <random>

// MACROS =============================================================================

// Named colors
#define PIECE_COLOR 1
#define PAUSE_COLOR 2

// A shortcut to set and unset colors to a screen
#define wset_color(screen, color) wattron(screen, COLOR_PAIR(color))
#define wunset_color(screen, color) wattroff(screen, COLOR_PAIR(color))

// STATIC VARIABLES ===================================================================
std::string tetromino[7];        // The game pieces
int nFieldWidth = 12;            // Width of the gaming field
int nFieldHeight = 18;           // Height of the gaming field
unsigned char *pField = nullptr; // The gaming fiels will be stored as a char array
uint nScreenWidth = 42;          // Width of the main window
uint nScreenHeight = 23;         // Height of the main window
WINDOW *screen,                  // Windows used to display stuff
       *scoreScreen, 
       *nextPieceScreen,
       *specialScreen;         
int ch;                          // Variable to manage user input

// HELPER FUNCTIONS ===================================================================

// To apply rotation to a piece, instead of rotating the piece itself, we translate
// the coordinates. This way we can easily create a new piece without having to
// manage each piece alone, saving some precious memory
int Rotate(int px, int py, int r){
  switch  (r % 4) {
    case 0: return (py << 2) + px;
    case 1: return 12 + py - (px << 2);
    case 2: return 15 - (py << 2) - px;
    case 3: return 3 - py + (px << 2);
  }
  return 0;
}

// Collision detection. A rather simple proceeding, we just compare the position in the
// tetromino with the position it'll occupy in the field. If the tetromino has an 'X' and
// the field has anything but a blank, then it will collide and thus block the piece in place
bool DoesPieceFit(int nTetromino, int nRotation, int nPosX, int nPosY) {
  for(int px = 0; px < 4; ++px) {
    for(int py = 0; py < 4; ++py) {
      // Get piece index
      int pi = Rotate(px, py, nRotation);

      // Get field index
      int fi = (nPosY + py) * nFieldWidth + (nPosX + px);

      // Check bounds
      if(nPosX + px >= 0 && nPosX + px < nFieldWidth) {
        if(nPosY + py >= 0 && nPosY + py < nFieldHeight) {
          if (tetromino[nTetromino][pi] == 'X' && pField[fi] != 0) return false;
        }
      }
    }
  }
  return true;
}

// Helper function to create a boxed window using ncurses
WINDOW *create_newwin(int height, int width, int starty, int startx) {
  WINDOW *local_win;
  local_win = newwin(height, width, starty, startx);
  box(local_win, 0 , 0);  /* 0, 0 gives default characters 
                             for the vertical and horizontal
                             lines   */
  wrefresh(local_win);  /* Show that box   */
  return local_win;
}

// Helper function to destroy a window using ncurses
void destroy_win(WINDOW *local_win) {
  werase(local_win);
  //wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
  wrefresh(local_win);
  delwin(local_win);
}

// I don't really need to explain this one, do I? DO I?
int main(int argc, char** argv) {
  
  // Initializing locale for using unicode chars
    setlocale(LC_ALL, "");

  // Game assets. Each tetromino is a 4 x 4 shape stored
  // as a string. A 'X' denotes a filled cell and a '.' 
  // denotes a hole
  tetromino[0].append("..X...X...X...X."); // Vertical bar
  tetromino[1].append(".X...X...XX....."); // L shape
  tetromino[2].append("..X...X..XX....."); // Inverted L shape
  tetromino[3].append(".X...XX...X....."); // S shape
  tetromino[4].append("..X..XX..X......"); // Z shape
  tetromino[5].append("..X..XX...X....."); // T shape
  tetromino[6].append(".....XX..XX....."); // O shape

  // Initializing the gaming field. It is stored as a unsigned char array
  // with values from 0 to 9. The code is:
  // 0 - denotes a blank space
  // 1 to 7 - denotes each tetromino shape
  // 8 - used to mark full lines to be removed
  // 9 - denotes the borders of the gaming field
  // In the initialization, we just fill it with either 0s or 9s, as it
  // initially has only the borders and blank spaces
  pField  = new unsigned char[nFieldWidth*nFieldHeight];
  for (uint x = 0; x < nFieldWidth; ++x) {
    for (uint y = 0; y < nFieldHeight; ++y) {
      pField[y*nFieldWidth + x] = (x == 0 || x == nFieldWidth -1 || y == nFieldHeight-1) ? 9 : 0;
    }
  }

  // Ncurses mode initialization
  initscr();            // Start curses mode
  curs_set(0);          // No ugly cursor in my game
  noecho();             // Do not echo characters to the screen, I'll put them manually
  cbreak();             // Line buffering disabled, Pass on  every thing to me
  keypad(stdscr, TRUE); // I need that nifty F1
  timeout(0);           // getch() non-blocking mode
  start_color();			  // Start color support
  
  // Initializing colors
	init_pair(PIECE_COLOR, COLOR_BLACK, COLOR_WHITE);
	init_pair(PAUSE_COLOR, COLOR_YELLOW, COLOR_BLUE);
  refresh();            /* Refresh the main screen */ 

  // Creating the main screen
  screen = create_newwin(nScreenHeight, nScreenWidth, 0, 0);
  
  mvwprintw(screen, 0, 7, "TETRIS Command Line Edition");
  mvwaddch(screen, nScreenHeight - 3, 2, ACS_LARROW);
  mvwaddch(screen, nScreenHeight - 3, 4, ACS_DARROW);
  mvwaddch(screen, nScreenHeight - 3, 6, ACS_RARROW);
  mvwprintw(screen, nScreenHeight - 3, 8, "to move, z to rotate, p to pause,");
  mvwprintw(screen, nScreenHeight - 2, 2, "space to use special, q to quit.");
    
  // Creating a window to display the score
  scoreScreen = create_newwin(3, 12, 2, 17);
  mvwprintw(scoreScreen, 0, 1, "Score");
  mvwprintw(scoreScreen, 1, 10, "0");

  // Creating a window to display the next piece
  nextPieceScreen = create_newwin(7, 8, 5, 17);
  mvwprintw(nextPieceScreen, 0, 1, "Next");

  // Creating a window to display available specials
  specialScreen = create_newwin(3, 9, 5, 26);
  mvwprintw(specialScreen, 0, 1, "Special");

  // Initializing random engine
  std::random_device generator;
  std::uniform_int_distribution<int> distribution(0,6);

  // Game state variables
  int nCurrentPiece = distribution(generator); //rand() % 7;      // We initialize the current piece randomly
  int nNextPiece = distribution(generator); //rand() % 7;         // And also the next piece as well
  int nCurrentRotation = 0;            // All pieces start in their unrotated state
  int nCurrentX = nFieldWidth / 2 - 2; // This will put the piece centered in the field
  int nCurrentY = 0;                   // And at the first line. The (0,0) of the piece is the upper left cell

  int nSpeed = 60;       // The initial game speed. Decreasing this will make the game harder
  int nSpeedCounter = 0; // A counter to increase the speed at determined moments
  uint nPieceCount = 0;  // A counter to store how many pieces were put on the field

  bool bForceDown = false; // A control variable to force the piece to move

  bool bRotateHold = false; // A control variable to block continuous rotation if the player holds the key

  bool bGameOver = false;  // State of the game: playing or not
  bool bPaused = false;    // State of the game: paused
  bool bPauseHold = false; // Control variable the block continuous pause

  std::vector<int> vLines; // A vector to store the Y coordinate of full lines

  uint nScore = 0; // The game score

  uint nSpecial = 0; // 'Swap' special power, allows you to swap current for next piece
  uint nRest = 0;
  uint nScoreCounter = 0;

  std::chrono::milliseconds gameTick(16);  // The game ticks at 16ms (roughly 60fps)
  std::chrono::milliseconds lineWait(400); // A pause of roughly half a second to display a line removal animation

  // GAME MAIN LOOP =============================================================================

  /* The game main loop is divided in sections:
     TIMING: here we take care of timing related routines. Remember each computer is different,
             so we need to standardize how we account the time. The chrono namespace has a lot
             of good utilities for this.
     INPUT:  this section manages the user input. As this game is very simple, using an event
             driven approach would be overkill, so we'll use good ol' polling.
     GAME LOGIC: in this section, we manage to do all the calculations for changing the game state
                 each frame.
     RENDER: finally, we concentrate drawing related stuff here.
     This approach is the most used by a variety of games. Obviously, sometimes we have to brake
     the hierarchy and perform game logic in the render section or vice-versa, depending of what
     we need to do. But for most of time, we can just follow the structure.
  */
  while(!bGameOver) {

    // TIMING ===================================================================================

    std::this_thread::sleep_for(gameTick);
    if (!bPaused) {
      nSpeedCounter++; // We count how many ticks have passed
      bForceDown = (nSpeedCounter == nSpeed); /* When we have as many ticks as the current
                                                 speed, we force the piece down a line */
    }

    // INPUT ====================================================================================

    ch = getch(); // Which key the player pressed in this tick? (non-blocking and buffered)

    // GAME LOGIC ===============================================================================

    // Checking the input and moving the piece accordingly
    int temp;
    switch(ch) {
      case 'p':
      case 'P': // The player wants to pause the game
        bPaused = !bPauseHold ? !bPaused : bPaused;
        bPauseHold = true;
        break;
      case 'q':
      case 'Q': // The player wants to quit. Case insensitive
        bGameOver = true;
        break;
      case KEY_LEFT: // Left arrow pressed, check collision and move the piece if possible
        if(DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX-1, nCurrentY)) {
          --nCurrentX;
        }
        break;
      case KEY_RIGHT: // Right arrow pressed
        if(DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX+1, nCurrentY)) {
          ++nCurrentX;
        }
        break;
      case KEY_DOWN: // Down arrow pressed
        if(DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1)) {
          ++nCurrentY;
        }
        break;
      case 'z':
      case 'Z': // Z key is used for rotation. Here, we also block the movement if the player
                // holds down the key, so it will always rotate once
        nCurrentRotation += (!bRotateHold && DoesPieceFit(nCurrentPiece, nCurrentRotation + 1, nCurrentX, nCurrentY)) ? 1 : 0;
        bRotateHold = true;
        break;
      case ' ': // Space key uses the special power, that swaps current and next piece
        if (nSpecial > 0){
          nSpecial--;
          temp = nCurrentPiece;
          nCurrentPiece = nNextPiece;
          nNextPiece = temp;
        }
        break;
      default: // Unblock rotation, if the rotation key is not hold down
        bRotateHold = false;
        bPauseHold = false;
        break;
    }

    // If the game is paused, we print a message and skip
    if (bPaused) {
      wset_color(screen, PAUSE_COLOR);
      mvwprintw(screen, nScreenHeight / 2, 3, " PAUSED!! ");
      wunset_color(screen, PAUSE_COLOR);
      wrefresh(screen);
      continue;
    }

    // Check if it is time to move the piece down the field
    if(bForceDown) {

      // If yes, just check for collision and move the piece
      if(DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1)) {
        nCurrentY++;
      } else {

        // If there is a collision, we have some things to check...
        // Lock the current piece into the field
        for(int px = 0; px < 4; px++){
          for(int py = 0; py < 4; py++) {
            if(tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] == 'X')
              pField[(nCurrentY + py) * nFieldWidth + (nCurrentX + px)] = nCurrentPiece + 1;
          }
        }

        // Increase piece count
        nPieceCount++;

        // Each 10 pieces we increase the speed a bit
        if (nPieceCount % 10 == 0){
          if (nSpeed >= 10) nSpeed--;
        }

        // Check for full lines. Here, we don't need to check the entire field, we just check
        // within the last piece played. We run over its height, checking if every cell along
        // the width is not blank. The full line is filled with a different charater and the
        // Y coordinate is pushed back to the vector
        for(int py = 0; py < 4; py++){
          if(nCurrentY + py < nFieldHeight - 1){
            bool bLine = true;
            for(int px = 1; px < nFieldWidth - 1; px++) {
              bLine &= (pField[(nCurrentY + py) * nFieldWidth + px]) != 0;
            }
            if(bLine){
              for(int px = 1; px < nFieldWidth - 1; px++) {
                pField[(nCurrentY + py) * nFieldWidth + px] = 8;
              }
              vLines.push_back(nCurrentY + py);
            }
          }
        }

        // Each piece gives 25 points when locked to the field
        nScore += 25;
        
        // Removing lines gives 2^n * 100 points, so the more lines removed, greater the reward
        if (!vLines.empty()) nScore += (1 << vLines.size()) * 100;

        // Choose next piece
        nCurrentX = nFieldWidth / 2 - 2; // Resetting the position
        nCurrentY = 0;
        nCurrentRotation = 0;
        nCurrentPiece = nNextPiece; // The next piece becomes the current
        nNextPiece = distribution(generator); //rand() % 7;    // And we sort a new one

        // If piece does not fit, game over
        bGameOver = !DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY);
      }
      nSpeedCounter = 0; // As the piece has moved, reset the counter
    }

    nRest = (nScore / 1000);
    if (nRest > nScoreCounter){
      int nGrowth = nRest - nScoreCounter;
      nSpecial += nSpecial < 3 ? nGrowth : 0;
      nScoreCounter += nGrowth;
    }

    // RENDER ===============================================================================================

    // Draw game field
    for(uint x =0; x < nFieldWidth; ++x) {
      for(uint y = 0; y < nFieldHeight; ++y) {

        // We draw a character based in the field coding. Here is a neat trick, we chose the
        // corresponding character based on the code. This helps to save some memory and cycles
        int i = pField[y * nFieldWidth + x];
        if (i > 0 && i < 8) wset_color(screen, PIECE_COLOR);
        mvwprintw(screen, y+2, x+2, "%c", " ABCDEFG=#"[i]);
        if (i > 0 && i < 8) wunset_color(screen, PIECE_COLOR);
      }
    }

    // Draw current piece. Here, we iterate over the current tetromino shape and check for filled cells
    // Before checking, we need to apply the rotation.    
    wset_color(screen, PIECE_COLOR);
    for(int px = 0; px < 4; ++px) {
      for(int py = 0; py < 4; ++py) {
        if(tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] == 'X') {
          mvwprintw(screen, (nCurrentY + py + 2), (nCurrentX + px + 2), "%c", nCurrentPiece + 65); // Nice trick :-)
        }
      }
    }
    wunset_color(screen, PIECE_COLOR);

    // Draw next piece in the Next window
    for(int px = 0; px < 4; ++px) {
      for(int py = 0; py < 4; ++py) {
        if(tetromino[nNextPiece][Rotate(px, py, 0)] == 'X'){
          wset_color(nextPieceScreen, PIECE_COLOR);
          mvwprintw(nextPieceScreen, (py + 2), (px + 2), "%c", nNextPiece + 65);
          wunset_color(nextPieceScreen, PIECE_COLOR);
        } else
          mvwprintw(nextPieceScreen, (py + 2), (px + 2), "%c", ' ');
      }
    }

    // Draw score
    int nDigits = (int) log10(nScore) + 1; // Trick to count how many digits the score has
    mvwprintw(scoreScreen, 1, 11 - nDigits, "%d", nScore); // The alignment is adjusted for the number of digits

    // Draw special
    mvwprintw(specialScreen, 1, 1, "       ");
    for(int i = 0; i < nSpecial; i++)
      mvwaddch(specialScreen, 1, 6-2*i, '*');

    // If we got any full line, vLines will not be empty. Time to remove them
    if(!vLines.empty()) {
      wrefresh(screen); // Refresh screen to show the line removal animation
      std::this_thread::sleep_for(lineWait); // We want to show it for about half a second

      // Pop lines and move down
      for(auto &l : vLines) { // Running over the stored coordinates
        for(int px = 1; px < nFieldWidth - 1; px++) { // and then, over the corresponding columns
          for(int py = l; py > 0; py--) {
            pField[py * nFieldWidth + px] = pField[(py-1) * nFieldWidth + px]; // and movin everything down
          }
          pField[px] = 0;
        }
      }
      vLines.clear();
    }       

    // Display the current frame. In this case, just refresh the windows
    wrefresh(screen);
    wrefresh(scoreScreen);
    wrefresh(nextPieceScreen);   
    wrefresh(specialScreen);     
  }

  // When the loop breaks, the game is over. Destroy the windows
  destroy_win(scoreScreen);
  destroy_win(nextPieceScreen);
  destroy_win(specialScreen);
  destroy_win(screen);
  refresh(); // Clear everything
  endwin(); // End curses mode and print the score before leaving
  std::cout << "###################################" << std::endl;
  std::cout << "# Game over... Score: " << nScore << std::endl;
  std::cout << "###################################" << std::endl;
  return 0; // Bye, bye
}