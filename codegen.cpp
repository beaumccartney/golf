#include "codegen.hpp"
#include "error.hpp"
#include <cassert>

static std::string output;

// label counters
static int global_labels   = 0,
           function_labels = 0,
           string_labels   = 1; // S0 is reserved for included null string

inline void asm_prologue()
{
    output +=
R"(
.text
# RTS FUNCTIONS

.globl main
main:
    jal Lmain
    j   halt

getchar:
    addi $sp, $sp, -4
    sw $ra, 0($sp)

    li $v0, 8                       # System call for read_string
    la $a0, getcharbuf              # Pass in buffer (size = n+1)
    li $a1, 2                       # Size = n+1 as read_string adds null
    syscall

    lb $v0, getcharbuf
    bne $v0, $zero, ret

    li $v0, -1                      # If 0, map to -1 and return
ret:
    lw $ra 0($sp)
    addi $sp $sp, 4
    jr $ra


prints:
    addi $sp, $sp, -4
    sw $ra, 0($sp)

    # $a0 contains the address of the string to print
    li $v0, 4                       # System call for print_string
    syscall

    lw $ra 0($sp)
    addi $sp $sp, 4
    jr $ra


# No need for stack here as
# the program will halt after this
halt:
    li $v0, 10
    syscall
    )";
}

inline void asm_epilogue()
{
    output +=
R"(
.data
getcharbuf:
        .space 2
        .align 2
S0:
        .byte 0
        .align 2
)";
}

inline void emitlabel
( std::string code )
{
    output += code + '\n';
}

inline void emitinstruction
( std::string instruction )
{
    output += '\t' + instruction + '\n';
}

inline std::string getlabel
( std::string prefix, int num )
{
    return prefix + std::to_string( num ) + ':';
}

void pass_1_pre( ASTNode & node )
{
    switch ( node.type )
    {
        case AST_STRING:
        {
            node.stringlabel = getlabel( "S", string_labels++ );
        } break;
        default: break;
    }
}

void pass_1_post( ASTNode & node )
{
}

void pass_2_cb( ASTNode & node )
{
    if ( node.type != AST_GLOBVAR && node.type != AST_STRING ) return;

    std::string label       = "LABEL UNSET";
    std::string instruction = "INSTRUCTION UNSET";

    if ( node.type == AST_GLOBVAR )
    {
        std::string type = node.symbolinfo->signature;
        instruction      = type == "string" ? ".word S0" : ".word 0" ;

        label = getlabel( "G", global_labels++ );
        node.symbolinfo->label = label;
    }
    else
    {
        label       = node.stringlabel;
        instruction = ".asciiz \"" + node.lexeme + "\"" ;
    }

    emitlabel( label );
    emitinstruction( instruction );
}

void gen_code( ASTNode & root )
{
    // EMIT PROLOGUE
    // - set up stuff and go to main
    asm_prologue();

    // pass 1 - code generation
    // prepost for everything else
    // funccall:
    //  - function prologue
    //  - jump and return address thing to function
    //  - function epilogue
    prepost( root, pass_1_pre, pass_1_post );

    asm_epilogue();

    // pass 2 - statically allocated things
    // preorder to emit global var declarations
    // - AST_GLOBVAR
    //    - a label plus either a word or a string constant
    // - AST_FUNC ( ????? )

    preorder( root, pass_2_cb );

    printf( "%s", output.data() );
}
