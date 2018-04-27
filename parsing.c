//
//  parsing.c
//  myLISP
//
//  Created by Nnamdi Kalu on 4/16/18.
//  Copyright © 2018 Nnamdi Kalu. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#include "mpc.h"

int main(int argc, char** argv) {
    
    /* initial parsers */
    mpc_parser_t* Number     = mpc_new("number");
    mpc_parser_t* Operator   = mpc_new("operator");
    mpc_parser_t* Expression = mpc_new("expression");
    mpc_parser_t* Phrase     = mpc_new("phrase");
    
    /* Defined as follows */
    mpca_lang(MPCA_LANG_DEFAULT,
              "                                                             \
              number : /-?[0-9]+/ ;                                         \
              operator : '+' | '-' | '*' | '/';                             \
              expression: <number> | '(' <operator>  <expression>+ ')';    \
              phrase: /^/ <operator> <expression>+ /$/;                     \
              ",
              Number, Operator, Expression, Phrase);
    
    /* Print Version and Exit info */
    puts("NnamLISP Version  0.0.0.1");
    puts("Press CTRL+C to Exit \n");
    
    while(1){
        char* input = readline("NnamLISP> ");
        
        /* adding command to history */
        add_history(input);
        
        
        /* attempt to parse user input */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Phrase, &r)){
            mpc_ast_print(r.output);
            mpc_ast_delete(r.output);
        }else{
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        
        free(input);
    }
    
    mpc_cleanup(4, Number, Operator, Expression, Phrase);
    
    return 0;
}
