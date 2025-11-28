#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef enum
{
    TOKEN_EOF,
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,

    TOKEN_EQUAL,
    TOKEN_SEMICOLON,
    TOKEN_LPAREN,
    TOKEN_RPAREN,

    TOKEN_PRINT
} TokenType;

typedef struct
{
    TokenType type;
    const char *start;
    int length;
} Token;

typedef enum
{
    EXPR_NUMBER,
    EXPR_BINARY
} ExprType;

typedef struct Expr
{
    ExprType type;
    union
    {
        struct
        {
            double value;
        } number;

        struct
        {
            struct Expr *left;
            TokenType op;
            struct Expr *right;
        } binary;
    } as;
} Expr;

typedef struct
{
    const char *start;
    const char *current;
} Lexer;

static Lexer lexer; // global lexer

void init_lexer(const char *source)
{
    lexer.start = source;
    lexer.current = source;
}

// Helper funcs
static char peek(void)
{
    return *lexer.current;
}

static char advance(void)
{
    return *lexer.current++;
}
static int is_at_end(void)
{
    return *lexer.current == '\0';
}

static void skip_whitespace(void)
{
    for (;;)
    {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            advance();
        }
        else
        {
            break;
        }
    }
}

static Token make_token(TokenType type)
{
    Token token;
    token.type = type;
    token.start = lexer.start;
    token.length = (int)(lexer.current - lexer.start);
    return token;
}

static Token error_token(const char *message)
{
    Token token;
    token.type = TOKEN_EOF;
    token.start = message;
    token.length = (int)strlen(message);
    return token;
}

static Token number(void)
{
    while (isdigit((unsigned char)peek()))
    {
        advance();
    }

    return make_token(TOKEN_NUMBER);
}

static is_alpha_or_underscore(char c)
{
    return isalpha((unsigned char)c) || c == '_';
}

static Token identifier(void)
{
    while (is_alpha_or_underscore(peek()) || isdigit((unsigned char)peek()))
    {
        advance();
    }

    int length = (int)(lexer.current - lexer.start);

    // Check for keyword
    if (length == 5 && strncmp(lexer.start, "print", 5) == 0)
    {
        return make_token(TOKEN_PRINT);
    }

    return make_token(TOKEN_IDENTIFIER);
}

Token scan_token(void)
{
    skip_whitespace();

    lexer.start = lexer.current;

    if (is_at_end())
    {
        return make_token(TOKEN_EOF);
    }

    char c = advance();

    if (isdigit((unsigned char)c))
    {
        return number();
    }

    if (is_alpha_or_underscore(c))
    {
        return identifier();
    }

    switch (c)
    {
    case '+':
        return make_token(TOKEN_PLUS);
    case '-':
        return make_token(TOKEN_MINUS);
    case '*':
        return make_token(TOKEN_STAR);
    case '/':
        return make_token(TOKEN_SLASH);
    case '=':
        return make_token(TOKEN_EQUAL);
    case ';':
        return make_token(TOKEN_SEMICOLON);
    case '(':
        return make_token(TOKEN_LPAREN);
    case ')':
        return make_token(TOKEN_RPAREN);
    }

    return error_token("Unexpected character.");
}

const char *token_type_name(TokenType type)
{
    switch (type)
    {
    case TOKEN_EOF:
        return "EOF";
    case TOKEN_NUMBER:
        return "NUMBER";
    case TOKEN_IDENTIFIER:
        return "IDENTIFIER";
    case TOKEN_PLUS:
        return "PLUS";
    case TOKEN_MINUS:
        return "MINUS";
    case TOKEN_STAR:
        return "STAR";
    case TOKEN_SLASH:
        return "SLASH";
    case TOKEN_EQUAL:
        return "EQUAL";
    case TOKEN_SEMICOLON:
        return "SEMICOLON";
    case TOKEN_LPAREN:
        return "LPAREN";
    case TOKEN_RPAREN:
        return "RPAREN";
    case TOKEN_PRINT:
        return "PRINT";
    default:
        return "UNKNOWN";
    }
}

// Parser
static Expr *new_number_expr(double value)
{
    Expr *expr = (Expr *)malloc(sizeof(Expr));
    if (!expr)
    {
        fprintf(stderr, "Out of memory in new_number_expr\n");
        exit(1);
    }

    expr->type = EXPR_NUMBER;
    expr->as.number.value = value;

    return expr;
}

int main(void)
{
    char buffer[4096];

    printf("Enter a line of code (e.g., 'x = 1 + 2 * 3; print x;'):\n");

    if (!fgets(buffer, sizeof(buffer), stdin))
    {
        fprintf(stderr, "Error reading input.\n");
        return 1;
    }

    init_lexer(buffer);

    for (;;)
    {
        Token token = scan_token();

        printf("%s: '", token_type_name(token.type));
        for (int i = 0; i < token.length; i++)
        {
            putchar(token.start[i]);
        }
        printf("'\n");

        if (token.type == TOKEN_EOF)
        {
            break;
        }
    }

    return 0;
}
