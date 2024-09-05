#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <ncurses.h>

// draw screen and at the end flush curses internal buffers to the screen

bool editor_exit = false;

// this structure should implement all the features needed in the editor
// - random insert/delete, cursor tracking, searching
// also: we would like this to not have a huge memory overhead since we intend to edit big files

// struct Line {
//   std::vector<char> line; // maybe implement by myself??
//   Line(): line(0) {}

//   // line ne spune unde sa desenam, begin_poz ne spune ce interval sa afisam din string
//   void draw( int line, int begin_poz ) {
    
//   }
// };

const int FILE_BUFSIZ = 20000;
const int MAX_LINES = FILE_BUFSIZ;

struct EditBuffer {
  char *buf; // extra space for '\0'
  int bufpoz;
  int cursor;

  // buf has bufpoz characters, buf[bufpoz] == '\0' allways, 0 <= cursor <= bufpoz

  EditBuffer(): bufpoz(0), cursor(0) {
    buf = new char[FILE_BUFSIZ + 1];
    buf[0] = '\0';
  }

  ~EditBuffer() {
    delete []buf;
  }

  void insert_char( char ch, int poz ) {
    if( poz < 0 || poz > bufpoz )
      return;
    
    if( bufpoz >= FILE_BUFSIZ ) // make sure to not go over buffer limit (no resizing yet)
      return;
    
    memmove( buf + poz + 1, buf + poz, (bufpoz + 1) - (poz) );
    buf[poz] = ch;
    bufpoz++;
  }
  
  void insert_char_cursor( char ch ) {
    insert_char( ch, cursor );
    cursor++; // daca insert_char nu face nimic asta e o greseala, schimb mai tarziu..
  }

  void remove_char( int poz ) {
    if( poz < 0 || poz >= bufpoz )
      return;
    
    memmove( buf + poz, buf + poz + 1, (bufpoz + 1) - (poz + 1) );
    bufpoz--;
  }

  // removes the character to the left of the cursor
  void backspace() {
    if( cursor <= 0 )
      return;
    
    cursor--;
    remove_char( cursor );
  }

  // cursor movement
  void cursor_left() {
    if( cursor <= 0 )
      return;

    cursor--;
  }

  void cursor_right() {
    if( cursor + 1 > bufpoz )
      return;

    cursor++;
  }

  void cursor_up() {}
  void cursor_down() {}

  // draws the buffer onto the screen, will add:
  //  - frame start cursor
  //  - frame coordinates (startl, startc, width, height)
  void draw() {
    move( 0, 0 );
    for( int i = 0; i < cursor; i++ )
      addch( buf[i] );

    // save cursor position
    int cursorx, cursory;
    getyx( stdscr, cursorx, cursory );

    for( int i = cursor; i < bufpoz; i++ )
      addch( buf[i] );

    move( cursorx, cursory );
  }
};

EditBuffer test_buf;

char bigboss = '?';
void draw() {
  clear();

  // not in main since window size can change
  int width, height;
  getmaxyx( stdscr, height, width );

  // process new key events
  int ch;
  //int iter = 0;
  while( /* iter < 10 && */ (ch = getch()) != EOF ) {
    //iter++;

    switch( ch ) {
    case KEY_BACKSPACE:
    case 127:
      test_buf.backspace();
      break;

    case KEY_UP:
      test_buf.cursor_up();
      break;

    case KEY_DOWN:
      test_buf.cursor_down();
      break;

    case KEY_LEFT:
      test_buf.cursor_left();
      break;

    case KEY_RIGHT:
      test_buf.cursor_right();
      break;
      
    case '\e':
      editor_exit = true;
      break;

    default:
      test_buf.insert_char_cursor( ch );
      bigboss = ch;
      break;
    }
  }

  move( height - 1, 0 );
  printw( "Terminal size: %dx%d (bigboss = %c | %d)\n", height, width, bigboss, int(bigboss) );

  // draw status line
  for( int c = 0; c < width; c++ )
    mvaddch( height - 2, c, '-' );

  test_buf.draw();
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
