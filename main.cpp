#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <ncurses.h>

template<typename T> T min( T a, T b ) { return a < b ? a : b; }
template<typename T> T max( T a, T b ) { return a > b ? a : b; }

template<typename T, const int HARD_LIMIT> struct LimitVector {
  static const int MIN_NONZERO_SIZ = 8;
  T *mem;
  int mem_siz;
  int siz;

  LimitVector(): mem(nullptr), mem_siz(0), siz(0) {}
  LimitVector( int siz ): mem_siz(siz), siz(siz) {
    mem = new T[siz];
  }

  ~LimitVector() {
    if( mem )
      delete []mem;
  }

  LimitVector& operator = ( const LimitVector& that ) {
    if( mem )
      delete []mem;

    mem_siz = that.mem_siz;
    siz = that.siz;

    if( that.mem ){
      mem = new T[mem_siz];
      for( int i = 0; i < siz; i++ )
        mem[i] = that.mem[i];
    }

    return *this;
  }

  T& operator []( const int idx ) {
    //assert( 0 <= idx < siz );
    return mem[idx];
  }

  T& back() {
    //assert( siz > 0 );
    return mem[siz - 1];
  }
  
  inline int size() { return siz; }
  inline bool empty() { return !siz; }

  void clear() {
    if( mem )
      delete []mem;

    mem = nullptr;
    mem_siz = 0;
    siz = 0;
  }

  void expand() {
    assert( mem_siz < HARD_LIMIT );
    
    int new_siz = min( max( MIN_NONZERO_SIZ, mem_siz * 2 ), HARD_LIMIT );
    T *new_mem = new T[new_siz];

    for( int i = 0; i < siz; i++ )
      new_mem[i] = mem[i];
    
    if( mem )
      delete []mem;
    mem = new_mem;
    mem_siz = new_siz;
  }

  void push_back( T el ) {
    if( siz >= mem_siz )
      expand();

    mem[siz++] = el;
  }

  void pop_back() {
    //assert( siz > 0 );
    siz--;
  }

  void insert( int idx, T el ) {
    //assert( 0 <= idx <= siz );
    if( siz >= mem_siz )
      expand();

    for( int i = siz - 1; i >= idx; i-- ) // memmove
      mem[i + 1] = mem[i];
    mem[idx] = el;
    siz++;
  }

  void erase( int idx ) {
    //assert( 0 <= idx < siz );

    for( int i = idx; i + 1 < siz; i++ ) // memmove
      mem[i] = mem[i + 1];
    siz--;
  }
};

bool editor_exit = false;

// this structure should implement all the features needed in the editor
// - random insert/delete, cursor tracking, searching
// also: we would like this to not have a huge memory overhead since we intend to edit big files

const int MAX_LINE_SIZ = 2000;
struct Line {
  LimitVector<char, MAX_LINE_SIZ> line;
  Line(): line(0) {}

  Line& operator = ( const Line& that ) {
    line = that.line;
    return *this;
  }

  // i just don't want to do inheritance
  inline void insert( int idx, char ch ) { line.insert( idx, ch ); }
  inline void erase( int idx ) { line.erase( idx ); }
  inline int size() { return line.size(); }
  inline void pop_back() { line.pop_back(); }

  // line ne spune unde sa desenam, begin_poz ne spune ce interval sa afisam din string
  void draw( int y, int begin_poz, int len ) {
    move( y, 0 );

    int end_poz = min( begin_poz + len, (int)line.size() );
    for( int idx = begin_poz; idx < end_poz; idx++ )
      addch( line[idx] );
  }
};

const int FILE_BUFSIZ = 20000;
const int MAX_LINES = FILE_BUFSIZ;

// this structure should eventualy also hold information about how to save the buffer (fillepath/format etc..)
struct EditBuffer {
  LimitVector<Line, MAX_LINES> lines;

  int cursor_line;
  int cursor_poz;

  int frame_begin; // linia de unde incepe afisarea

  char *output;

  EditBuffer(): lines(1), cursor_line(0), cursor_poz(0), frame_begin(0), output(nullptr) { lines[0] = Line(); }
  EditBuffer( const char *filepath ): lines(), cursor_line(0), cursor_poz(0), frame_begin(0) {
    int siz = 0;
    while( filepath[siz] )
      siz++;
    
    // copy to output
    output = new char[siz + 1];
    for( int i = 0; i <= siz; i++ )
      output[i] = filepath[i];

    lines.push_back( Line() );
    
    // check if file allready exists
    if( access( output, F_OK ) == 0 ){ // belongs to <unistd.h>
      // file exists, must load into lines[]
      FILE *fin = fopen( output, "r" );
      char ch;

      while( (ch = fgetc( fin )) != EOF ){
        if( ch == '\n' )
          lines.push_back( Line() );
        else
          lines.back().line.push_back( ch );
      }

      fclose( fin );
    }
  }

  void flush() {
    if( !output )
      return;

    FILE *fout = fopen( output, "w" );
    for( int i = 0; i < lines.size(); i++ ){
      if( i )
        fputc( '\n', fout );

      for( int j = 0; j < lines[i].size(); j++ )
        fputc( lines[i].line[j], fout );
    }
    fclose( fout );
  }

  ~EditBuffer() {
    flush();
    if( output )
      delete []output;
  }

  void normalize_cursor() {
    cursor_poz = min( lines[cursor_line].size(), cursor_poz );
  }
  
  void insert_char_cursor( char ch ) {
    normalize_cursor();
    
    if( ch == '\n' ){
      lines.insert( cursor_line + 1, Line() );

      for( int i = cursor_poz; i < lines[cursor_line].size(); i++ )
        lines[cursor_line + 1].line.push_back( lines[cursor_line].line[i] );

      // don't want to do this by hand just because
      for( ; lines[cursor_line].size() > cursor_poz ; )
        lines[cursor_line].pop_back();
      
      cursor_line++;
      cursor_poz = 0;
      return;
    }
    
    lines[cursor_line].insert( cursor_poz++, ch );
  }

  // removes the character to the left of the cursor
  void backspace() {
    normalize_cursor();

    if( cursor_poz > 0 ){
      cursor_poz--;
      lines[cursor_line].erase( cursor_poz );
      return;
    }

    if( cursor_line <= 0 )
      return;

    cursor_poz = lines[cursor_line - 1].size();
    cursor_line--;

    for( int i = 0; i < lines[cursor_line + 1].size(); i++ )
      lines[cursor_line].line.push_back( lines[cursor_line + 1].line[i] );
    lines.erase( cursor_line + 1 );
  }

  // cursor movement
  void cursor_left() {
    normalize_cursor();
    if( cursor_line <= 0 && cursor_poz <= 0 )
      return;
    
    if( cursor_poz > 0 )
      cursor_poz--;
    else{
      cursor_line--;
      cursor_poz = lines[cursor_line].size();
    }
  }

  void cursor_right() {
    normalize_cursor();
    if( (cursor_line + 1 >= lines.size()) && (cursor_poz >= lines[cursor_line].size()) )
      return;

    if( cursor_poz + 1 <= lines[cursor_line].size() )
      cursor_poz++;
    else{
      cursor_line++;
      cursor_poz = 0;
    }
  }

  void cursor_up() {
    if( cursor_line <= 0 )
      return;

    cursor_line--;
  }

  void cursor_down() {
    if( cursor_line + 1 >= lines.size() )
      return;

    cursor_line++;
  }

  void update_frame( int height ) {
    while( cursor_line < frame_begin )
      frame_begin = min( 0, frame_begin - height );

    while( frame_begin + height <= cursor_line )
      frame_begin += height;
  }

  // draws the buffer onto the screen, will add:
  //  - frame start cursor
  //  - frame coordinates (startl, startc, width, height)
  void draw( int width, int height ) {
    update_frame( height );
    move( 0, 0 );

    int last_line = min( lines.size(), frame_begin + height );
    for( int i = frame_begin; i < last_line; i++ )
        lines[i].draw( i - frame_begin, 0, width );

    move( cursor_line - frame_begin, min( lines[cursor_line].size(), cursor_poz ) );
  }
};

EditBuffer *test_buf;

#define ctrl(x) ((x) & 0x1f)

char bigboss = '?';
void handle_io() {
  // process new key events
  int ch;
  //int iter = 0;
  while( /* iter < 10 && */ (ch = getch()) != ERR ) {
    //iter++;

    switch( ch ) {
    case KEY_BACKSPACE:
    case 127:
      test_buf->backspace();
      break;

    case KEY_UP:
      test_buf->cursor_up();
      break;

    case KEY_DOWN:
      test_buf->cursor_down();
      break;

    case KEY_LEFT:
      test_buf->cursor_left();
      break;

    case KEY_RIGHT:
      test_buf->cursor_right();
      break;

    case ctrl('S'):
      test_buf->flush();
      break;

    case ctrl('C'):
    case '\e':
      editor_exit = true;
      break;

    default:
      test_buf->insert_char_cursor( ch );
      bigboss = ch;
      break;
    }
  }
}

// draw screen and at the end flush curses internal buffers to the screen
void draw() {
  // not in main since window size can change
  int width, height;
  getmaxyx( stdscr, height, width );

  erase(); // clear();
  move( height - 1, 0 );
  printw( "Terminal size: %dx%d (bigboss = %c | %d)\n", height, width, bigboss, int(bigboss) );

  // draw status line
  for( int c = 0; c < width; c++ )
    mvaddch( height - 2, c, '-' );

  test_buf->draw( width, height - 2 ); // we don't want the buffer to touch the status line
  refresh();
}

int main( int argc, char **argv ) {
  initscr();
  noecho(); // we don't want to not be able to control what is on the screen

  // change to raw() when we dont want Ctrl+C, Ctrl+D to do anything
  //cbreak();
  raw();

  timeout( 10 ); // non-blocking read (getch())
  keypad( stdscr, true );

  // at the end we move the cursor to its position in the buffer
  //curs_set( 0 ); // invisible cursor

  if( argc > 1 )
    test_buf = new EditBuffer( argv[1] );
  else
    test_buf = new EditBuffer();
  
  do{
    handle_io();
    draw();
    //usleep( 1000 * 20 ); // 20ms delay ~ 50fps
  }while( !editor_exit );

  endwin();

  // buf[bufpoz] = '\0';
  // printf( "%s\n", buf );
  delete test_buf;
  return 0;
}
