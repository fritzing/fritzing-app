
%parser XMLTable

%impl xmlreader.cpp

%token LEFT_ANGLE
%token RIGHT_ANGLE
%token ANY

%start XmlStream

/.
#ifndef XMLREADER_H
#define XMLREADER_H

#include <QtCore>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "$header"

class XMLReader: protected $table
{
public:
    XMLReader(const QByteArray &bytes);
    ~XMLReader();

    bool parse();

    inline int nextToken()
    {
        switch (*bufptr++) {
        case '\0':
            return EOF_SYMBOL;

        case '<':
            in_tag = true;
            return LEFT_ANGLE;

        case '>':
            if (! in_tag)
                break;
            in_tag = false;
            return RIGHT_ANGLE;
            break;

        } // switch

        return ANY;
    }

protected:
    inline void reallocateStack();

    inline int &sym(int index)
    { return stack [tos + index - 1].ival; }

protected:
    int tos;
    int stack_size;

    struct StackItem {
        int state;
        int ival;
    };

    QVarLengthArray<StackItem> stack;
    unsigned in_tag: 1;
    QByteArray bytes;
    const char *bufptr;
};

inline void XMLReader::reallocateStack()
{
    if (! stack_size)
        stack_size = 128;
    else
        stack_size <<= 1;

    stack.resize (stack_size);
}

#endif // XMLREADER_H

XMLReader::XMLReader(const QByteArray &bytes):
    tos(0),
    stack_size(0),
    bytes(bytes)
{
    bufptr = bytes.constData();
}

XMLReader::~XMLReader()
{
}

bool XMLReader::parse()
{
  const int INITIAL_STATE = 0;

  in_tag = 0;
  bufptr = bytes.constData();

  int yytoken = -1;
  reallocateStack();

  tos = 0;
  stack [++tos].state = INITIAL_STATE;

  while (true)
    {
      const int state = stack [tos].state;

      if (yytoken == -1 && - TERMINAL_COUNT != action_index [state])
        yytoken = nextToken();

      int act = t_action (state, yytoken);

      if (act == ACCEPT_STATE)
        return true;

      else if (act > 0)
        {
          if (++tos == stack_size)
            reallocateStack();

          stack [tos].ival = *bufptr; // ### save the token value here
          stack [tos].state = act;
          yytoken = -1;
        }

      else if (act < 0)
        {
          int r = - act - 1;

          tos -= rhs [r];
          act = stack [tos++].state;

          switch (r) {
./




XmlStream: TagOrWord ;
XmlStream: XmlStream TagOrWord ;

TagOrWord: Tag ;
TagOrWord: ANY ;

Tag: LEFT_ANGLE TagName RIGHT_ANGLE ;
/.
    case $rule_number: {
        fprintf (stderr, "*** found a tag\n");
    } break;
./

TagName: ANY ;
TagName: TagName ANY ;


/.
          } // switch

          stack [tos].state = nt_action (act, lhs [r] - TERMINAL_COUNT);
        }

      else
        {
          // ### ERROR RECOVERY HERE
          break;
        }
    }

    return false;
}



/////////////////////////////
// entry point
/////////////////////////////
int main(int, char *argv[])
{
    QFile f (argv[1]);

    if (f.open(QFile::ReadOnly)) {
        QByteArray contents = f.readAll();
        XMLReader parser (contents);

        if (parser.parse())
            printf ("OK\n");
        else
            printf ("KO\n");
    }
}




./

