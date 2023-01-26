#include <assert.h>
#include "lex.hpp"
#include "error.hpp"
#include <queue>

bool is_space(int c) { return c == '\n' || c == '\t' || c == ' ' || c == '\r'; }

bool is_letter(int c) { return isalpha(c) || c == '_'; }

std::queue<Tokinfo> tokens;

void unlex(Tokinfo t) { tokens.push(t); }

int g_linenum = 1;
Tokinfo lex(FILE *fp) {

    Tokinfo result;

    // prioritize tokens in the queue
    if (!tokens.empty()) {
        result = tokens.front();
        tokens.pop();
        return result;
    }

    // put result's members in local scope
    auto & token = result.token;

    spin:
    int ch;
    // spin until we find something interesting
    while ( ( ch = getc(fp) ) != EOF && is_space(ch) )
        if (ch == '\n') g_linenum++;

    result.linenum = g_linenum;
    result.lexeme  = ch; // first (and possibly only) character of the lexeme

    switch (ch) {
        // NULL character isn't allowed in input
        case NULL:
            error("NULL character found - not permitted", g_linenum);
        case EOF:
            // TODO: insert semicolon where appropriate
            token = TOKEN_EOF;
            break;
        case '+':
            token = TOKEN_PLUS;
            break;
        case '-':
            token = TOKEN_MINUS;
            break;
        case '*':
            token = TOKEN_STAR;
            break;
        case '%':
            token = TOKEN_PERCENT;
            break;
        case '(':
            token = TOKEN_LPAREN;
            break;
        case ')':
            token = TOKEN_RPAREN;

            // peek next char to see if its a newline. if so, insert a semicolon
            if ( ( ch = getc(fp) ) == '\n' )
                unlex(Tokinfo {TOKEN_SEMICOLON, g_linenum, "",});

            // put back to mimick "peeking"
            ungetc(ch, fp);
            break;
        case '{':
            token = TOKEN_LBRACE;
            break;
        case '}':
            // TODO: if this closes an open brace and the last token wasn't a semicolon, insert a semicolon???
            token = TOKEN_RBRACE;

            // peek next char to see if its a newline. if so, insert a semicolon
            if ( ( ch = getc(fp) ) == '\n' )
                unlex(Tokinfo {TOKEN_SEMICOLON, g_linenum, "",});

            // put back to mimick "peeking"
            ungetc(ch, fp);
            break;
        case ',':
            token = TOKEN_COMMA;
            break;
        case ';':
            token = TOKEN_SEMICOLON;
            break;
        case '&':
            // no unary & operator
            if ( (ch = getc(fp)) != '&' ) error("& operator not supported - maybe you wanted &&?", g_linenum);
            token = TOKEN_LOGIC_AND;
            result.lexeme.push_back(ch);
            break;
        case '|':
            // no unary | operator
            if ( (ch = getc(fp)) != '|' ) error("| operator not supported- maybe you wanted ||?", g_linenum);
            token = TOKEN_LOGIC_AND;
            result.lexeme.push_back(ch);
            break;
        // TODO: fix line number bug with these cases
        case '=':
            // if '=' is next, then it's == comparison, else =
            if ((ch = getc(fp)) == '=') {
              token = TOKEN_EQ;
              result.lexeme.push_back(ch);
            } else {
              token = TOKEN_ASSIGN;
              ungetc(ch, fp);
            }
            break;
        case '<':
            // if '=' is next, then it's <= comparison, else =
            if ((ch = getc(fp)) == '=') {
              token = TOKEN_LEQ;
              result.lexeme.push_back(ch);
            } else {
              token = TOKEN_LT;
              ungetc(ch, fp);
            }
            break;
        case '>':
            // if '=' is next, then it's >= comparison, else =
            if ((ch = getc(fp)) == '=') {
              token = TOKEN_GEQ;
              result.lexeme.push_back(ch);
            } else {
              token = TOKEN_GT;
              ungetc(ch, fp);
            }
            break;
        case '!':
            // if '=' is next, then it's >= comparison, else =
            if ((ch = getc(fp)) == '=') {
              token = TOKEN_NEQ;
              result.lexeme.push_back(ch);
            } else {
              token = TOKEN_BANG;
              ungetc(ch, fp);
            }
            break;
        case '/':
            // REVIEW: maybe make this look better?
            // single slash
            if ( ( ch = getc(fp) ) != '/' ) {
                token = TOKEN_SLASH;
                ungetc(ch, fp);
                break;
            }
            // comment - spin until we hit newline or EOF then lex at that newline/EOF
            while ( ( ch = getc(fp) ) != EOF && ch != '\n' )
                ;
            ungetc(ch, fp);
            goto spin;
        case '"':
            while ( ( ch = getc(fp) ) != EOF && ch != '"' ) {
                if (ch == '\n') error("Newline in string literal", g_linenum);
                // on backslash: check for valid escape sequences
                if (ch == '\\') {
                    ch = getc(fp);
                    if ( !(ch == 'b' || ch == 'f' ||ch == 'n' ||ch == 'r' ||ch == 't' ||ch == '\\' ||ch == '"') )
                        error("Invalid escape sequence", g_linenum);
                }
                result.lexeme += ch;
            }
            if (ch == EOF) error("EOF in string literal", g_linenum);
            token = TOKEN_STRING;
            break;
        default:
            if (is_space(ch)) goto spin;
            if (is_letter(ch)) {
                while ( ( ch = getc(fp) ) != EOF && isalnum(ch) )
                    result.lexeme += ch;
            }
            warning("skipping unknown character", g_linenum);
            goto spin;
            break;
    }

    return result;
}

const char *token_to_string(TokenID token) {
    switch (token) {
        case TOKEN_EOF:       return "EOF";
        case TOKEN_BREAK:     return "break";
        case TOKEN_ELSE:      return "else";
        case TOKEN_FOR:       return "for";
        case TOKEN_FUNC:      return "func";
        case TOKEN_IF:        return "if";
        case TOKEN_RETURN:    return "return";
        case TOKEN_VAR:       return "var";
        case TOKEN_PLUS:      return "+";
        case TOKEN_MINUS:     return "-";
        case TOKEN_STAR:      return "*";
        case TOKEN_SLASH:     return "/";
        case TOKEN_PERCENT:   return "%";
        case TOKEN_LOGIC_AND: return "&&";
        case TOKEN_LOGIC_OR:  return "||";
        case TOKEN_EQ:        return "==";
        case TOKEN_LT:        return "<";
        case TOKEN_GT:        return ">";
        case TOKEN_ASSIGN:    return "=";
        case TOKEN_BANG:      return "!";
        case TOKEN_NEQ:       return "!=";
        case TOKEN_LEQ:       return "<=";
        case TOKEN_GEQ:       return ">=";
        case TOKEN_LPAREN:    return "(";
        case TOKEN_RPAREN:    return ")";
        case TOKEN_LBRACE:    return "{";
        case TOKEN_RBRACE:    return "}";
        case TOKEN_COMMA:     return ",";
        case TOKEN_SEMICOLON: return ";";
        case TOKEN_ID:        return "id";
        case TOKEN_INT:       return "int";
        case TOKEN_STRING:    return "string";
        case TOKEN_UNSET:     return "DEBUG";
    }
}
