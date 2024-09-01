#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <ncurses.h>

// draw screen and at the end flush curses internal buffers to the screen

void draw() {
  clear();
  move( 0, 0 ); // start drawing scene from the top-left

  if( false && (clock() & 1) )
    printw( "Hello World !!!" );
  else
    printw( "Hello World 2 !!!" );

  refresh();
}

int main( int argc, char **argv ) {
  initscr();
  cbreak(); // change to raw() when we dont want Ctrl+C, Ctrl+D to do anything
  noecho(); // we don't want to not be able to control what is on the screen

  do{
    draw();
    usleep( 10000 );
  }while( true );

  endwin();
  return 0;
}
