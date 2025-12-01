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

static int is_alpha_or_underscore(char c)
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

#define MAX_VARS 64

typedef struct
{
    char name[64];
    double value;
    int is_set;
} Var;

static Var vars[MAX_VARS];

static Var *get_var_slot(const char *name, int length)
{
    for (int i = 0; i < MAX_VARS; i++)
    {
        if (vars[i].is_set)
        {
            if ((int)strlen(vars[i].name) == length && strncmp(vars[i].name, name, length) == 0)
            {
                return &vars[i];
            }
        }
    }

    for (int i = 1; i < MAX_VARS; i++)
    {
        if (!vars[i].is_set)
        {
            int copy_len = length;
            if (copy_len >= (int)sizeof(vars[i].name))
            {
                copy_len = (int)sizeof(vars[i].name) - 1;
            }

            memcpy(vars[i].name, name, copy_len);
            vars[i].name[copy_len] = '\0';
            vars[i].is_set = 1;
            vars[i].value = 0.0;
            return &vars[i];
        }
    }

    fprintf(stderr, "Too many variables (limit %d)\n", MAX_VARS);
    exit(1);
}

typedef enum
{
    STMT_EXPR,
    STMT_PRINT,
    STMT_ASSIGN
} StmtType;

typedef struct Stms
{
    StmtType type;
    union
    {
        struct
        {
            Expr *expr;
        } expr_stmt;

        struct
        {
            Expr *expr;
        } print_stmt;

        struct
        {
            char name[64];
            Expr *expr;
        } assign_stmt;
    } as;

    struct Stmt *next;

} Stmt;

typedef struct
{
    Token current;
} Parser;

static Parser parser; // globar parser

static void init_parser(void)
{
    parser.current = scan_token();
}

static void advance_parser(void)
{
    parser.current = scan_token();
}

// Helper for parser
static int match(TokenType type)
{
    if (parser.current.type == type)
    {
        advance_parser();
        return 1;
    }

    return 0;
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

static Expr *new_binary_expr(Expr *left, TokenType op, Expr *right)
{
    Expr *expr = (Expr *)malloc(sizeof(Expr));
    if (!expr)
    {
        fprintf(stderr, "Out of memory in new_binary_expr");
        exit(1);
    }

    expr->type = EXPR_BINARY;
    expr->as.binary.left = left;
    expr->as.binary.op = op;
    expr->as.binary.right = right;

    return expr;
}

static void expect(TokenType type, const char *message)
{
    if (parser.current.type == type)
    {
        advance_parser();
        return;
    }

    fprintf(stderr, "Parser error: %s\n", message);
    exit(1);
}

static Expr *parse_expression(void);

static Expr *parser_factor(void)
{
    if (parser.current.type == TOKEN_NUMBER)
    {
        char temp[64];
        int len = parser.current.length;
        if (len >= (int)sizeof(temp))
        {
            len = (int)sizeof(temp) - 1;
        }

        memcpy(temp, parser.current.start, len);
        temp[len] = '\0';

        double value = strtod(temp, NULL);
        advance_parser();

        return new_number_expr(value);
    }

    if (match(TOKEN_LPAREN))
    {
        Expr *inside = parse_expression();
        expect(TOKEN_RPAREN, "Expected ')' after expression");
        return inside;
    }

    fprintf(stderr, "Parser error: expected number or '(' \n");
    exit(1);
}

static Expr *parser_term(void)
{
    Expr *expr = parser_factor();

    // for * or /
    while (parser.current.type == TOKEN_STAR || parser.current.type == TOKEN_SLASH)
    {
        TokenType op = parser.current.type;
        advance_parser();
        Expr *right = parser_factor();
        expr = new_binary_expr(expr, op, right);
    }

    return expr;
}

static Expr *parse_expression(void)
{
    Expr *expr = parser_term();

    // + -
    while (parser.current.type == TOKEN_PLUS || parser.current.type == TOKEN_MINUS)
    {
        TokenType op = parser.current.type;
        advance_parser();
        Expr *right = parser_term();
        expr = new_binary_expr(expr, op, right);
    }

    return expr;
}

static Stmt *parser_statement(void);
static Stmt *parser_program(void);

static Stmt *new_stmt(StmtType type)
{
    Stmt *stmt = (Stmt *)malloc(sizeof(Stmt));

    if (!stmt)
    {
        fprintf(stderr, "Out of memory in new_stmt'n");
        exit(1);
    }

    stmt->type = type;
    stmt->next = NULL;
    return stmt;
}

static Stmt *parser_statement(void)
{
    if (parser.current.type == TOKEN_PRINT)
    {
        advance_parser();
        Expr *expr = parse_expression();
        expect(TOKEN_SEMICOLON, "Expected ';' after print expression");

        Stmt *stmt = new_stmt(STMT_PRINT);
        stmt->as.print_stmt.expr = expr;
        return stmt;
    }

    if (parser.current.type == TOKEN_IDENTIFIER)
    {
        Token name_token = parser.current;
        advance_parser();

        if (match(TOKEN_EQUAL))
        {
            Expr *expr = parse_expression();
            expect(TOKEN_SEMICOLON, "Expected ';' after assignment");

            Stmt *stmt = new_stmt(STMT_ASSIGN);

            int len = name_token.length;
            if (len >= (int)sizeof(stmt->as.assign_stmt.name))
            {
                len = (int)sizeof(stmt->as.assign_stmt.name) - 1;
            }
            memcpy(stmt->as.assign_stmt.name, name_token.start, len);
            stmt->as.assign_stmt.name[len] = '\0';

            stmt->as.assign_stmt.expr = expr;

            return stmt;
        }
        else
        {
            fprintf(stderr, "Parser error: expected '=' after identifier for assignt \n");
            exit(1);
        }
    }

    Expr *expr = parse_expression();
    expect(TOKEN_SEMICOLON, "Expected ';' after exoression");

    Stmt *stmt = new_stmt(STMT_EXPR);
    stmt->as.expr_stmt.expr = expr;
    return stmt;
}

static Stmt *parser_program(void)
{
    Stmt *head = NULL;
    Stmt *tail = NULL;

    while (parser.current.type != TOKEN_EOF)
    {
        Stmt *stmt = parser_statement();
        if (head == NULL)
        {
            head = tail = stmt;
        }
        else
        {
            tail->next = stmt;
            tail = stmt;
        }
    }

    return head;
}

// Evaluation of expression tree
static double eval_expr(Expr *expr)
{
    switch (expr->type)
    {
    case EXPR_NUMBER:
        return expr->as.number.value;

    case EXPR_BINARY:
    {
        double left = eval_expr(expr->as.binary.left);
        double right = eval_expr(expr->as.binary.right);
        switch (expr->as.binary.op)
        {
        case TOKEN_PLUS:
            return left + right;
        case TOKEN_MINUS:
            return left - right;
        case TOKEN_STAR:
            return left * right;
        case TOKEN_SLASH:
            return left / right;
        default:
            fprintf(stderr, "Runtime error: unknown binary operator\n");
            exit(1);
        }
    }
    }

    fprintf(stderr, "Runtime error: unknown expression type.\n");
    exit(1);
}

int main(void)
{
    char buffer[4096];

    printf("Enter an arithmetic expression (e.g., 1 + 2 * (3 + 4)):\n");

    if (!fgets(buffer, sizeof(buffer), stdin))
    {
        fprintf(stderr, "Error reading input.\n");
        return 1;
    }

    init_lexer(buffer);
    init_parser();

    Expr *expr = parse_expression();

    // Optionally: we could check for EOF here
    if (parser.current.type != TOKEN_EOF &&
        parser.current.type != TOKEN_SEMICOLON)
    {
        fprintf(stderr, "Unexpected extra input after expression.\n");
        return 1;
    }

    double result = eval_expr(expr);
    printf("Result = %g\n", result);

    return 0;
}
