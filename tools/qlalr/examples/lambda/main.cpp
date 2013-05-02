
#include "lambda.h"


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "parser_table_p.h"

class Parser: protected parser_table
{
public:
    union Value {
      int ival;
      // ### more...
    };

public:
    Parser();
    ~Parser();

    bool parse();

protected:
    inline void reallocateStack();

    inline Value &sym(int index)
    { return sym_stack [tos + index - 1]; }

    int nextToken();
    void consumeRule(int ruleno);

protected:
    int tos;
    int stack_size;
    Value *sym_stack;
    int *state_stack;
    int current_char;
    unsigned in_tag: 1;
};

inline void Parser::reallocateStack()
{
    if (! stack_size)
        stack_size = 128;
    else
        stack_size <<= 1;

    sym_stack = reinterpret_cast<Value*> (::realloc(sym_stack, stack_size * sizeof(Value)));
    state_stack = reinterpret_cast<int*> (::realloc(state_stack, stack_size * sizeof(int)));
}

Parser::Parser():
    tos(0),
    stack_size(0),
    sym_stack(0),
    state_stack(0)
{
}

Parser::~Parser()
{
    if (stack_size) {
        ::free(sym_stack);
        ::free(state_stack);
    }
}

bool Parser::parse()
{
  const int INITIAL_STATE = 0;

  current_char = 0;
  in_tag = 0;

  int yytoken = -1;
  reallocateStack();

  tos = 0;
  state_stack[++tos] = INITIAL_STATE;

  while (true)
    {
      if (yytoken == -1 && - TERMINAL_COUNT != action_index [state_stack [tos]])
        yytoken = nextToken();

      int act = t_action (state_stack [tos], yytoken);

      if (act == ACCEPT_STATE) {
        return true;
      }

      else if (act > 0)
        {
          if (++tos == stack_size)
            reallocateStack();

          sym_stack [tos].ival = current_char; // ### save the token value here
          state_stack [tos] = act;
          yytoken = -1;
        }

      else if (act < 0)
        {
          int r = - act - 1;

          tos -= rhs [r];
          act = state_stack [tos++];
          consumeRule (r);
          state_stack [tos] = nt_action (act, lhs [r] - TERMINAL_COUNT);
        }

      else
        break;
    }

    return false;
}


int Parser::nextToken()
{
    static int tokens[] = { ID, ID, ID, EOF_SYMBOL };
    static int *tk = tokens;

    return *tk++;
}

void Parser::consumeRule(int ruleno)
{
    switch (ruleno) {
    case Symbol:
        printf("symbol\n");
        break;
    case SubExpression:
        printf("sub-expr\n");
        break;
    case Appl:
        printf("appl\n");
        break;
    case Abstr:
        printf("abstr\n");
        break;
    }
}

/////////////////////////////
// entry point
/////////////////////////////
int main()
{
    Parser parser;

    if (parser.parse())
        printf ("OK\n");
    else
        printf ("KO\n");
}


