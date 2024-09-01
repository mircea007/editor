#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <ncurses.h>

#define false 0
#define true 1
#define bool int

// draw screen and at the end flush curses internal buffers to the screen

#define FILE_BUFSIZ 20000

char buf[FILE_BUFSIZ + 1]; // extra space for '\0'
int bufpoz = 0;
int cursor = 0;

bool editor_exit = false;

void draw() {
  clear();

  // not in main since window size can change
  int width, height;
  getmaxyx( stdscr, height, width );

  // process new key events
  int ch;
  char bigboss = '?';
  //int iter = 0;
  while( /* iter < 10 && */ (ch = getch()) != EOF ) {
    //iter++;

    switch( ch ) {
    case KEY_BACKSPACE:
    case 127:
      if( cursor > 0 ){
        cursor--;
        memmove( buf + cursor, buf + cursor + 1, (bufpoz + 1) - (cursor + 1) );
        bufpoz--;
      }
      break;

    case KEY_UP:
      break;

    case KEY_DOWN:
      break;

    case KEY_LEFT:
      if( cursor > 0 )
        cursor--;
      break;

    case KEY_RIGHT:
      if( cursor < bufpoz )
        cursor++;
      break;
      
    case '\e':
      //editor_exit = true;
      break;

    default:
      if( bufpoz < FILE_BUFSIZ ){
        memmove( buf + cursor + 1, buf + cursor, (bufpoz + 1) - (cursor) );
        buf[cursor++] = ch;
        bufpoz++;
      }
      bigboss = ch;
      //exit( 1 ); // sa vedem daca se ajunge aici
      break;
    }
  }

  move( 0, 0 );
  printw( "Terminal size: %dx%d (bigboss = %c | %d)\n", height, width, bigboss, int(bigboss) );

  // draw status line
  for( int c = 0; c < width; c++ )
    mvaddch( height - 2, c, '-' );

  buf[bufpoz] = '\0';
  move( 1, 0 );

  for( int i = 0; i < cursor; i++ )
    addch( buf[i] );

  int cursorx, cursory;
  getyx( stdscr, cursorx, cursory );

  for( int i = cursor; i < bufpoz; i++ )
    addch( buf[i] );

  move( cursorx, cursory );

  refresh();
}

int main( int argc, char **argv ) {
  initscr();
  noecho(); // we don't want to not be able to control what is on the screen

  // change to raw() when we dont want Ctrl+C, Ctrl+D to do anything
  cbreak();
  //raw();

  timeout( 0 ); // non-blocking read (getch())
  keypad( stdscr, true );
  
  do{
    draw();
    usleep( 1000 * 20 ); // 20ms delay ~ 50fps
  }while( !editor_exit );

  endwin();

  // buf[bufpoz] = '\0';
  // printf( "%s\n", buf );
  return 0;
}
