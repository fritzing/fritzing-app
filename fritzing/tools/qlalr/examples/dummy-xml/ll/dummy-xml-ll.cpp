
#include <cstdlib>
#include <cstdio>

enum Token {
    EOF_SYMBOL,
    LEFT_ANGLE,
    RIGHT_ANGLE,
    ANY,
};

static int current_char;
static int yytoken;
static bool in_tag = false;

bool parseXmlStream();
bool parseTagOrWord();
bool parseTagName();

inline int nextToken()
{
    current_char = fgetc(stdin);
    if (current_char == EOF) {
        return (yytoken = EOF_SYMBOL);
    } else if (current_char == '<') {
        in_tag = true;
        return (yytoken = LEFT_ANGLE);
    } else if (in_tag && current_char == '>') {
        in_tag = false;
        return (yytoken = RIGHT_ANGLE);
    }
    return (yytoken = ANY);
}

bool parse()
{
    nextToken();
    return parseXmlStream();
}

bool parseXmlStream()
{
    while (parseTagOrWord())
        ;

    return true;
}

bool parseTagOrWord()
{
    if (yytoken == LEFT_ANGLE) {
        nextToken();
        if (! parseTagName())
            return false;
        if (yytoken != RIGHT_ANGLE)
            return false;
        nextToken();

        fprintf (stderr, "*** found a tag\n");

    } else if (yytoken == ANY) {
        nextToken();
    } else {
        return false;
    }
    return true;
}

bool parseTagName()
{
    while (yytoken == ANY)
        nextToken();

    return true;
}

int main()
{
    if (parse())
        printf("OK\n");
    else
        printf("KO\n");
}
