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

/* declare lval(LISP value) struct */
typedef struct{
    int type;
    long num;
    int err;    
} lval;

/* create enumeration of possible lval types*/
enum {LVAL_NUM, LVAL_ERR};

/* enumeration of possible error values */
enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM}; 

/* create a new number lval type */
lval lval_num(long x){
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

/* create a new error lval type */
lval lval_err(int x){
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
} 

/* print an lval */
void lval_print(lval v){
    switch(v.type){

        case(LVAL_NUM): 
            printf("%li \n", v.num);
            break;

        case(LVAL_ERR):
            if(v.err == LERR_DIV_ZERO){
                printf("Divide By Zero Error \n");
            }
            else if(v.err == LERR_BAD_NUM){
                printf("Invalid Number Error \n");
            }
            else if(v.err == LERR_BAD_OP){
                printf("Invalid Operator Error \n");
            }
            else{
                printf("Unknown Error \n");
            }
            break;
    }
} 


lval eval_op(lval x, char* op, lval y){

    if (x.type == LVAL_ERR){ return x;}
    if (y.type == LVAL_ERR){ return y;}

    if (strcmp(op, "+") == 0) {return lval_num(x.num + y.num);}
    if (strcmp(op, "-") == 0) {return lval_num(x.num - y.num);}
    if (strcmp(op, "*") == 0) {return lval_num(x.num * y.num);}
    if (strcmp(op, "/") == 0) {
        /* divide by zero error check */
        return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
    }
    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t){

    /* return numbers directly */
    if (strstr(t->tag, "number")){
        /* check for error in conversion */
        errno = 0; 
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    char* op = t->children[1]->contents;
    lval x = eval(t->children[2]);

    /* iterate through remaining children */
    int i = 3;
    while (strstr(t->children[i]->tag, "expression")){
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;

}

int main(int argc, char** argv) {
    
    /* initial parsers */
    mpc_parser_t* Number     = mpc_new("number");
    mpc_parser_t* Operator   = mpc_new("operator");
    mpc_parser_t* Expression = mpc_new("expression");
    mpc_parser_t* Phrase     = mpc_new("phrase");
    
    /* Defined as follows */
    mpca_lang(MPCA_LANG_DEFAULT,
              "                                                                 \
              number    : /-?[0-9]+/ ;                                          \
              operator  : '+' | '-' | '*' | '/';                                \
              expression: <number> | '(' <operator>  <expression>+ ')';         \
              phrase    : /^/ <operator> <expression>+ /$/;                     \
              ",
              Number, Operator, Expression, Phrase);
    
    /* Print Version and Exit info */
    puts("NnamLISP Version  0.0.0.3");
    puts("Press CTRL+C to Exit \n");
    
    while(1){
        char* input = readline("NnamLISP> ");
        
        /* adding command to history */
        add_history(input);
        
        
        /* attempt to parse user input */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Phrase, &r)){
            lval result = eval(r.output);
            lval_print(result);
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
