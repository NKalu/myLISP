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
typedef struct lval {
    int type;
    long num;
    /* Char for Error and Symbol Types */
    char* err;
    char* sym;

    /* Pointer to list of "lval*" */
    int count;
    struct lval** cell;    
} lval;

/* create enumeration of possible lval types*/
enum {LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPRE};

void lval_print(lval* v);
/* Contstruct pointer to Number lval */
lval* lval_num(long x){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

/* Contstruct pointer to Error lval */
lval* lval_err(char* y){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(y) + 1);
    strcpy(v->err, y);
    return v;
} 

/* Contstruct pointer to Symbol; lval */
lval* lval_sym(char* z){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(z) + 1);
    strcpy(v->sym, z);
    return v;
}

/* Contstruct pointer to SExpression lval */
lval* lval_sexpre(void){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPRE;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_read_num(mpc_ast_t* t){
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval* lval_add(lval* v, lval* x){
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count - 1] = x;
    return v;
}

lval* lval_read(mpc_ast_t* t){

    if (strstr(t->tag, "number")) {return lval_read_num(t);}
    if (strstr(t->tag, "symbol")) {return lval_sym(t->contents);}

    /* empty list if root or sexpression*/
    lval* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpre();}
    if (strstr(t->tag, "sexpression")) { x = lval_sexpre();}

    /* Fill list with valid expressions */
    for (int i = 0; i < t->children_num; i++){
        if (strcmp(t->children[i]->contents, "(") == 0) {continue;}
        if (strcmp(t->children[i]->contents, ")") == 0) {continue;}
        if (strcmp(t->children[i]->tag,  "regex") == 0) {continue;}
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

/* call to free for "lval*"" */
void lval_del(lval* v){
    
    switch(v->type){
        case LVAL_NUM: break;

        /* Free strings */
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;

        case LVAL_SEXPRE:
            for (int i = 0; i < v->count; i++){
                lval_del(v->cell[i]);
            }
            break;
    }
    /* Free mempory for lval struct */
    free(v);
}

void lval_expr_print(lval* v, char open, char close){
    putchar(open);
    for(int i = 0; i < v->count; i++){

        lval_print(v->cell[i]);

        if (i != (v->count-1)){
            putchar(' ');
        }
    }
    putchar(close);
}

/* print an lval */
void lval_print(lval* v){
    switch(v->type){
        case LVAL_NUM:    printf("%li", v->num); break;
        case LVAL_ERR:    printf("Error: %s", v->err); break;
        case LVAL_SYM:    printf("%s", v->sym); break;
        case LVAL_SEXPRE: lval_expr_print(v, '(', ')'); break;
    }
} 

void lval_println(lval* v){
    lval_print(v);
    putchar('\n');
}


// lval eval_op(lval x, char* op, lval y){

//     if (x.type == LVAL_ERR){ return x;}
//     if (y.type == LVAL_ERR){ return y;}

//     if (strcmp(op, "+") == 0) {return lval_num(x.num + y.num);}
//     if (strcmp(op, "-") == 0) {return lval_num(x.num - y.num);}
//     if (strcmp(op, "*") == 0) {return lval_num(x.num * y.num);}
//     if (strcmp(op, "/") == 0) {
//         /* divide by zero error check */
//         return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
//     }
//     return lval_err(LERR_BAD_OP);
// }

// lval eval(mpc_ast_t* t){

//     /* return numbers directly */
//     if (strstr(t->tag, "number")){
//         /* check for error in conversion */
//         errno = 0; 
//         long x = strtol(t->contents, NULL, 10);
//         return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
//     }

//     char* op = t->children[1]->contents;
//     lval x = eval(t->children[2]);

//     /* iterate through remaining children */
//     int i = 3;
//     while (strstr(t->children[i]->tag, "expression")){
//         x = eval_op(x, op, eval(t->children[i]));
//         i++;
//     }

//     return x;

// }

int main(int argc, char** argv) {
    
    /* initial parsers */
    mpc_parser_t* Number      = mpc_new("number");
    mpc_parser_t* Symbol      = mpc_new("symbol");
    mpc_parser_t* Sexpression = mpc_new("sexpression");
    mpc_parser_t* Expression  = mpc_new("expression");
    mpc_parser_t* Phrase      = mpc_new("phrase");
    
    /* Defined as follows */
    mpca_lang(MPCA_LANG_DEFAULT,
              "                                                                  \
              number     : /-?[0-9]+/ ;                                          \
              symbol     : '+' | '-' | '*' | '/';                                \
              sexpression: '(' <expression>* ')';                                \
              expression : <number> | <symbol>|  <sexpression>+ ')';             \
              phrase     : /^/ <operator> <expression>+ /$/;                     \
              ",
              Number, Symbol, Sexpression, Expression, Phrase);
    
    /* Print Version and Exit info */
    puts("NnamLISP Version  0.0.0.3");
    puts("Press CTRL+C to Exit \n");
    
    while(1){
        char* input = readline("NnamLISP> ");
        
        /* adding command to history */
        add_history(input);
        
        
        /* attempt to parse user input */
        mpc_result_t r;
        lval* x = lval_read(r.output);
        lval_println(x);
        lval_del(x);
        // if (mpc_parse("<stdin>", input, Phrase, &r)){
        //     // lval result = eval(r.output);
            
        // }else{
        //     mpc_err_print(r.error);+
        //     mpc_err_delete(r.error);
        // }
        
        free(input);
    }
    
    mpc_cleanup(5, Number, Symbol, Sexpression, Expression, Phrase);
    
    return 0;
}
