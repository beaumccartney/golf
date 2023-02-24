// TODO: use newid and not new id in the right places

#include "parse.hpp"
#include "lex.hpp"
#include "error.hpp"

ASTNode Expression();
ASTNode Block();
ASTNode UnaryExpr();
ASTNode Statement();

Tokinfo expect( TokenID type ) {
    auto tok = lex();

    if ( tok.token != type )
        error( tok.linenum, "expected token of type %s", token_to_string( type ) );

    return tok;
}

ASTNode newidentifier() {
    auto tok = expect( TOKEN_ID );

    return ASTNode ( AST_NEWID, tok.linenum, tok.lexeme );
}

ASTNode typeidentifier() {
    auto tok = expect( TOKEN_ID );

    return ASTNode ( AST_TYPEID, tok.linenum, tok.lexeme );
}

ASTNode BreakStmt() {
    auto tok = expect( TOKEN_BREAK );

    return ASTNode ( AST_BREAK, tok.linenum, tok.lexeme );
}

ASTNode ReturnStmt() {
    auto tok = expect( TOKEN_RETURN );

    return ASTNode ( AST_RETURN, tok.linenum, tok.lexeme );
}

ASTNode IfStmt() {
    auto tok = expect( TOKEN_IF );

    ASTNode result;

    result.linenum = tok.linenum;
    result.lexeme = tok.lexeme;
    result.type = AST_IF;

    result.add_child( Expression() );
    result.add_child( Block() );

    tok = lex();

    if ( tok.token != TOKEN_ELSE ) return result;


    result.type = AST_IFELSE;

    unlex( tok );
    if ( tok.token == TOKEN_LBRACE ) {
        result.add_child( Block() );
    } else if ( tok.token == TOKEN_IF ) {
        result.add_child( IfStmt() );
    } else {
        // TODO: make decent error
        error( tok.linenum, "" );
    }

    return result;
}

ASTNode ForStmt() {
    auto tok = expect( TOKEN_FOR );

    ASTNode result;

    // [ Condition ]
    if ( ( tok = lex() ).token != TOKEN_LBRACE ) result.add_child( Expression() );

    unlex( tok );
    result.add_child( Block() );

    return result;
}

ASTNode PL5() {
    ASTNode result = UnaryExpr();

    Tokinfo tok;
    while ( ( tok = lex() ).token == TOKEN_STAR || tok.token == TOKEN_SLASH || tok.token == TOKEN_PERCENT ) {
        auto right = UnaryExpr();
        auto left = result;
        result = ASTNode ( AST_LOGIC_OR, tok.linenum, tok.lexeme );

        result.add_child( left );
        result.add_child( right );
    }
    unlex( tok );

    return result;
}

ASTNode PL4() {
    ASTNode result = PL5();

    Tokinfo tok;
    while ( ( tok = lex() ).token == TOKEN_PLUS || tok.token == TOKEN_MINUS ) {
        auto right = PL5();
        auto left = result;
        result = ASTNode ( AST_LOGIC_OR, tok.linenum, tok.lexeme );

        result.add_child( left );
        result.add_child( right );
    }
    unlex( tok );

    return result;
}

ASTNode PL3() {
    ASTNode result = PL4();

    Tokinfo tok;
    while ( ( tok = lex() )
                  .token == TOKEN_EQ
            || tok.token == TOKEN_NEQ
            || tok.token == TOKEN_GEQ
            || tok.token == TOKEN_LEQ
            || tok.token == TOKEN_LT
            || tok.token == TOKEN_GT) {
        auto right = PL4();
        auto left = result;
        result = ASTNode ( AST_LOGIC_OR, tok.linenum, tok.lexeme );

        result.add_child( left );
        result.add_child( right );
    }
    unlex( tok );

    return result;
}

ASTNode PL2() {
    ASTNode result = PL3();

    Tokinfo tok;
    while ( ( tok = lex() ).token == TOKEN_LOGIC_AND ) {
        auto right = PL3();
        auto left = result;
        result = ASTNode ( AST_LOGIC_AND, tok.linenum, tok.lexeme );

        result.add_child( left );
        result.add_child( right );
    }
    unlex( tok );

    return result;
}

ASTNode PL1() {
    ASTNode result = PL2();

    Tokinfo tok;
    while ( ( tok = lex() ).token == TOKEN_LOGIC_OR ) {
        auto right = PL2();
        auto left = result;
        result = ASTNode ( AST_LOGIC_OR, tok.linenum, tok.lexeme );

        result.add_child( left );
        result.add_child( right );
    }
    unlex( tok );

    return result;
}


ASTNode Expression() {
    return PL1();
}

ASTNode Block() {
    expect( TOKEN_LBRACE );

    ASTNode result;
    result.type = AST_BLOCK;

    Tokinfo tok = lex();
    while ( tok.token != TOKEN_RBRACE ) {
        result.add_child( Statement() );
        expect( TOKEN_SEMICOLON );

        tok = lex();
    }

    unlex( tok );
    expect( TOKEN_RBRACE );

    return result;
}

// "formal" in the reference compiler I think
ASTNode ParameterDecl() {
    auto tok = expect( TOKEN_ID );

    ASTNode result ( AST_FORMAL );
    result.add_child( newidentifier() );
    result.add_child( typeidentifier() );

    return result;
}

ASTNode VarDecl() {
    auto tok = expect( TOKEN_VAR );

    ASTNode result ( AST_VAR, tok.linenum, tok.lexeme );

    result.add_child( newidentifier() );
    result.add_child( typeidentifier() );

    return result;
}

ASTNode FunctionDecl() {
    auto tok = expect( TOKEN_FUNC );

    ASTNode result ( AST_FUNC, tok.linenum, tok.lexeme );

    result.add_child( newidentifier() ); // FunctionName

    expect( TOKEN_LPAREN );

    // add signature
    ASTNode sig ( AST_SIGNATURE );
    ASTNode formals ( AST_FORMALS );

    tok = lex();
    // printf("token's lexeme: %s\n", tok.lexeme.data() );
    unlex( tok );
    // printf("token's lexeme: %s, %d\n", (tok = lex()).lexeme.data(), tok.token );

    if ( tok.token == TOKEN_ID ) {
        // ParameterList

        formals.add_child( ParameterDecl() );
        while ( ( tok = lex() ).token == TOKEN_COMMA )
            formals.add_child( ParameterDecl() );

        // consume trailing comma in params list if there is one
        if ( ( tok = lex() ).token != TOKEN_COMMA ) unlex( tok );
    }

    expect( TOKEN_RPAREN );

    sig.add_child( formals );

    tok = lex();
    unlex( tok );

    if ( tok.token == TOKEN_ID )
        sig.add_child( typeidentifier() );
    else {
        ASTNode inserted_void( AST_TYPEID, -1, "$void" );

        sig.add_child( inserted_void );
    }

    result.add_child( sig );

    result.add_child( Block() ); // FunctionBody

    return result;
}

ASTNode TopLevelDecl() {
    auto tok = lex();
    unlex( tok );

    ASTNode result;

    switch ( tok.token ) {
        case TOKEN_FUNC:
            result = FunctionDecl();
            break;
        case TOKEN_VAR:
            result = VarDecl();
            break;
        default:
            printf("current token is: %s\n", tok.lexeme.data());
            error( tok.linenum, "top level declarations must begin with 'func' or 'var'" );
    }

    return result;
}

ASTNode Operand() {
    auto tok = lex();

    switch ( tok.token ) {
        case TOKEN_INT:
            return ASTNode ( AST_INT, tok.linenum, tok.lexeme );
        case TOKEN_STRING:
            return ASTNode ( AST_STRING, tok.linenum, tok.lexeme );
        case TOKEN_ID:
            return ASTNode ( AST_ID, tok.linenum, tok.lexeme );
        default: {

            expect( TOKEN_LPAREN );
            auto result = Expression();
            expect( TOKEN_RPAREN );

            return result;
        }
    }
}

bool is_expression_next() {
    auto tok = lex();
    unlex( tok );

    return tok.token == TOKEN_BANG
        || tok.token == TOKEN_MINUS
        || tok.token == TOKEN_INT
        || tok.token == TOKEN_STRING
        || tok.token == TOKEN_ID
        || tok.token == TOKEN_LPAREN
    ;
}

ASTNode Arguments() {
    expect( TOKEN_LPAREN );
    ASTNode result = { AST_ACTUALS };

    if ( is_expression_next() ) {
        result.add_child( Expression() );
        Tokinfo tok;
        while ( ( tok = lex() ).token == TOKEN_COMMA )
            result.add_child( Expression() );

        // consume trailing comma in params list if there is one
        if ( ( tok = lex() ).token != TOKEN_COMMA ) unlex( tok );
    }

    expect( TOKEN_RPAREN );

    return result;
}

ASTNode Assignment() {
    ASTNode result = { AST_ASSIGN };
    result.add_child( Expression() );
    result.linenum = expect( TOKEN_ASSIGN ).linenum;
    result.add_child( Expression() );
    return result;
}

ASTNode ExpresstionStmt() {
    auto result = Expression();
    result.type = AST_EXPRSTMT;
    return result;
}

ASTNode Statement() {
    auto tok = lex();
    unlex( tok );

    switch ( tok.token ) {
        case TOKEN_VAR:
            return VarDecl();
        case TOKEN_RETURN:
            return ReturnStmt();
        case TOKEN_BREAK:
            return BreakStmt();
        case TOKEN_LBRACE:
            return Block();
        case TOKEN_IF:
            return IfStmt();
        case TOKEN_FOR:
            return ForStmt();
        default:
            if ( !is_expression_next() ) return { AST_EMPTYSTMT };
            auto result = ExpresstionStmt();
            tok = lex();
            if ( tok.token == TOKEN_ASSIGN ) {
                auto right = Expression();
                auto left = result;
                left.type = AST_EXPR;
                result = ASTNode ( AST_ASSIGN, tok.linenum, tok.lexeme );
                result.add_child( left );
                result.add_child( right );
            }
            unlex( tok );
            return result;
    }
}

ASTNode PrimaryExpr() {
    // TODO: figure out children situation ffs
    ASTNode result ( AST_EXPR );

    auto operand = Operand();
    result.add_child( operand );

    auto tok = lex();
    while ( ( tok = lex() ).token == TOKEN_LPAREN ) {
        unlex( tok );
        result.add_child( Arguments() );
    }
    unlex( tok );

    return result;
}

ASTNode UnaryExpr() {
    auto tok = lex();

    ASTNode result;

    switch ( tok.token ) {
        case TOKEN_BANG:
            result.type = AST_LOGIC_NOT;
            result.linenum = tok.linenum;
            result.add_child( UnaryExpr() );
            break;
        case TOKEN_MINUS:
            result.type = AST_UMINUS;
            result.linenum = tok.linenum;
            result.add_child( UnaryExpr() );
            break;
        default:
            unlex( tok );
            return PrimaryExpr();
    }

    return result;
}


// SourceFile
ASTNode parse() {
    ASTNode root;

    Tokinfo tok;

    while ( ( tok = lex() ).token != TOKEN_EOF ) {
        unlex( tok );
        auto decl = TopLevelDecl();
        root.add_child( decl );

        expect( TOKEN_SEMICOLON );
    }
    unlex( tok );

    return root;
}

const char * ASTNode_to_string( ASTNodeID n ) {
    switch (n) {
        default: return "foo";
    }

    return "bar";
}