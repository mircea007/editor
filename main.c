#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <ncurses.h>

#define false 0
#define true 1
#define bool int

// draw screen and at the end flush curses internal buffers to the screen

char buf[1000];
int bufpoz = 0;

bool editor_exit = false;

void draw() {
  clear();

  // not in main since window size can change
  int width, height;
  getmaxyx( stdscr, height, width );

  // process new key events
  char ch;
  char bigboss = '?';
  //int iter = 0;
  while( /* iter < 10 && */ (ch = getch()) != EOF ) {
    //iter++;

    if( ch == 'q' )
      editor_exit = true;
    
    if( ch == KEY_BACKSPACE || ch == 127 ){
      if( bufpoz >= 0 )
        bufpoz--;
    }else{
      if( bufpoz < 1000 )
        buf[bufpoz++] = ch;
      bigboss = ch;
      //exit( 1 ); // sa vedem daca se ajunge aici
    }
  }

  move( 0, 0 );
  printw( "Terminal size: %dx%d (bigboss = %c | %d)\n", height, width, bigboss, int(bigboss) );

  // draw status line
  for( int c = 0; c < width; c++ )
    mvaddch( height - 2, c, '-' );

  buf[bufpoz] = '\0';
  move( 1, 0 );
  char *ptr = buf;
  while( *ptr )
    addch( *(ptr++) );

  refresh();
}

int main( int argc, char **argv ) {
  initscr();
  cbreak(); // change to raw() when we dont want Ctrl+C, Ctrl+D to do anything
  noecho(); // we don't want to not be able to control what is on the screen

  timeout( 0 ); // non-blocking read (getch())

  do{
    draw();
    usleep( 1000 * 20 );
  }while( !editor_exit );

  endwin();

  buf[bufpoz] = '\0';
  printf( "%s\n", buf );
  return 0;
}
