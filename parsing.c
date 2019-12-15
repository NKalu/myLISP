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

#define LASSERT(args, cond, err) \
    if( !(cond) ) { lval_del(args); return lval_err(err); }

/* Forward Declarations */
struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;

/* create enumeration of possible lval types*/
enum {LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPRE, LVAL_QEXPRE, LVAL_FUN};

/* declare lval(LISP value) struct */
typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
    int type;
    long num;
    /* Char for Error and Symbol Types */
    char* err;
    char* sym;

    lbuiltin fun;

    /* Pointer to list of "lval*" */
    int count;
    struct lval** cell;    
};

void lval_print(lval* v);
lval* lval_eval(lenv* e, lval* v);
lval* builtin(lval* l, char* fun);

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

/* Contstruct pointer to QExpression lval */
lval* lval_qexpre(void){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPRE;
    v->count = 0;
    v->cell = NULL;
    return v;
}

/* Construct pointer for lbuiltin */
lval* lval_fun(lbuiltin func){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->fun = func;
    return v;
}

struct lenv {
    int count;
    char** syms;
    lval** vals;
};

lenv* lenv_new(void){
    lenv* e = malloc(sizeof(lenv));
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

void lenv_del(lenv* e){
    for (int i = 0; i < e->count; i++){
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

lval* lenv_get(lenv* e, lval* k){
    /* Loop over environment to look for symbol */
    for (int i = 0; i < e->count; i++){
        if(strcmp(e->syms[i], k->sym) == 0){
            /* return copy of value if match found */
            return lval_copy(e->vals[i]);
        }
    }
    return lval_err("unbound symbol");
}

void lenv_put(lenv* e, lval* k, lval* v){

    for (int i = 0; i < e->count; i++){
        if(strcmp(e->syms[i], k->sym) == 0){
            /* if var found, delete and replace */
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    /* allocate space for new entry */
    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    /* */
    e->vals[e->count-1] = lval_copy(v);
    e->syms[e->count-1] = malloc(strlen(k->sym)+1);
    strcpy(e->syms[e->count-1], k->sym);

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
    if (strstr(t->tag, "qexpression")) { x = lval_qexpre();}

    /* Fill list with valid expressions */
    for (int i = 0; i < t->children_num; i++){
        if (strcmp(t->children[i]->contents, "(") == 0) {continue;}
        if (strcmp(t->children[i]->contents, ")") == 0) {continue;}
        if (strcmp(t->children[i]->contents, "{") == 0) {continue;}
        if (strcmp(t->children[i]->contents, "}") == 0) {continue;}
        if (strcmp(t->children[i]->tag,  "regex") == 0) {continue;}
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

/* call to free for "lval*"" */
void lval_del(lval* v){
    
    switch(v->type){
        case LVAL_NUM:
        case LVAL_FUN: break;

        /* Free strings */
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;

        /* run for all in expression */
        case LVAL_QEXPRE:
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
        case LVAL_QEXPRE: lval_expr_print(v, '{', '}'); break;
        case LVAL_FUN:    printf("<function>"); break;
    }
} 

void lval_println(lval* v){
    lval_print(v);
    putchar('\n');
}

lval* lval_pop(lval* v, int i){
    /* find item at index i */
    lval* x = v->cell[i];

    /*shift memory after index */
    memmove(&v->cell[i], &v->cell[i+1], 
        sizeof(lval*) * (v->count - i - 1)); 

    v->count--;

    /* reallocate used memory */
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}

lval* lval_take(lval* v, int i){
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}

lval* lval_join(lval* x, lval* y){
    while(y->count){
        x = lval_add(x, lval_pop(y, 0));
    }

    /* delete empty Y and return X */
    lval_del(y);
    return x;
}

lval* lval_copy(lval* v){

    lval* x = malloc(sizeof(lval));
    x->type = v->type;

    switch(v->type){

        /* Copy functions and numbers directly */
        case LVAL_FUN: x->fun = v->fun; break;
        case LVAL_NUM: x->num = v->num; break;

        /* Copy strings(errors/symbols) with malloc/strcpy */
        case LVAL_ERR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err);
            break;

        case LVAL_SYM:
            x->sym = malloc(strlen(v->sym) + 1);
            strcpy(x->sym, v->sym);
            break;

        /* Copy Lists by copying sub-expression */
        case LVAL_SEXPRE:
        case LVAL_QEXPRE:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for(int i=0; i < x->count; i++){
                x->cell[i] = lval_copy(v->cell[i]);
            }
            break;
    }

    return x;
}

lval* builtin_op(lenv* e, lval* l, char* op){

    for(int i = 0; i < l->count; i++){
        if (l->cell[i]->type != LVAL_NUM){
            lval_del(l);
            return lval_err("Can only operate on numbers");
        }
    }


    /* get first element */
    lval* x = lval_pop(l, 0);

    /* if only on numer, just make negative if subtraction */
    if((strcmp(op, "-") == 0) && l->count == 0){
        x->num = -x->num;
    }

    while(l->count > 0){

        lval* y = lval_pop(l, 0);

        if (strcmp(op, "+") == 0) { x->num += y->num ;}
        if (strcmp(op, "-") == 0) { x->num -= y->num ;}
        if (strcmp(op, "*") == 0) { x->num *= y->num ;}
        if (strcmp(op, "/") == 0) { 
            if (y->num == 0){
                lval_del(x);
                lval_del(y);
                x = lval_err("Division By Zero Error");
                break;
            }
            x->num /= y->num; 
        }

        lval_del(y);
    }

    lval_del(l);
    return x;
}

lval* builtin_def(lenv* e, lval* a){
    LASSERT(a, a->cell[0]->type == LVAL_QEXPRE,
    "Cannot pass 'def' a Q-Expression");

    /* first arg should be list of symbols */
    lval* symbols = a->cell[0];

    /**/
    for(int i=0; i < symbols->count; i++){
        LASSERT(a, symbols->cell[i]->type == LVAL_SYM,
        "Only symbols may be passed to 'def'");
    }

    LASSERT(a, symbols->count == a->count-1,
    "Incorrect number of valus passed to 'def'")

    for(int i = 0; i < symbols->count; i++){
        lenv_put(e, symbols->cell[i], a->cell[i+1]);
    }

    lval_del(a);
    return lval_sexpre();
}

lval* builtin_add(lenv* e, lval* a){
    return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a){
    return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a){
    return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a){
    return builtin_op(e, a, "/");
}

lval* builtin_first(lenv* e, lval* l){
    /* check to make sure not too many args */
    LASSERT(l, l->count == 1,
        "Too many args passed to 'first'");
    /* check for qexpre */
    LASSERT(l, l->cell[0]->type == LVAL_QEXPRE,
        "Incorrect type passed to 'first'");
    /* check if empty */
    LASSERT(l, l->cell[0]->count != 0,
        "Empty q-expression passed to 'first'");

    /* take first arg */
    lval* v = lval_take(l, 0);
    /* delete everything else */
    while(v->count > 1){ 
        lval_del(lval_pop(v, 1));
    }

    return v;
}

lval* builtin_last(lenv* e, lval* l){
     /* check to make sure not too many args */
    LASSERT(l, l->count == 1,
        "Too many args passed to 'last'");
    /* check for qexpre */
    LASSERT(l, l->cell[0]->type == LVAL_QEXPRE,
        "Incorrect type passed to 'last'");
    /* check if empty */
    LASSERT(l, l->cell[0]->count != 0 ,
        "Empty q-expression passed to 'last'");

    /* take first arg */
    lval* v = lval_take(l, 0);
    /* delete first element */
    lval_del(lval_pop(v, 0));

    return v;
}

lval* builtin_list(lenv* e, lval* l){
    l->type = LVAL_QEXPRE;
    return l;
}

lval* builtin_eval(lenv* e, lval* l){
     /* check to make sure not too many args */
    LASSERT(l, l->count == 1,
        "Too many args passed to 'eval'");
    /* check for qexpre */
    LASSERT(l, l->cell[0]->type == LVAL_QEXPRE,
        "Incorrect type passed to 'eval'");

    /* take first arg, convert to s-expression and evaluate */
    lval* x = lval_take(l, 0);
    x->type = LVAL_SEXPRE;
    return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* l){
    /* check that everything is a q-expression */
    for(int i = 0; i < l->count; i++){
        LASSERT(l, l->cell[i]->type == LVAL_QEXPRE, 
            "Incorrect type passed to 'join'");
    }

    lval* x = lval_pop(l, 0);

    while(l->count){
        x = lval_join(x, lval_pop(l, 0));
    }

    lval_del(l);
    return x;
}

lval* lval_eval_sexpre(lenv* e, lval* v){

    /* eval children */
    for (int i = 0; i < v->count; i++){
        v->cell[i] = lval_eval(e, v->cell[i]);
    }

    /* error check */
    for (int i = 0; i < v->count; i++){
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    /* empty */
    if (v->count == 0){return v;}

    /* single expression */
    if (v->count == 1) { return lval_take(v, 0); }

    /* ensure first element is symbol */
    lval* f = lval_pop(v, 0);

    if (f->type != LVAL_FUN){
        lval_del(f); 
        lval_del(v);
        return lval_err("First Element is Not a Function");
    }

    /* call function to get result */
    lval* result = f->fun(e, v);
    lval_del(f);
    return result;

}

lval* lval_eval(lenv* e, lval* v){
    if (v->type == LVAL_SYM){
        lval* x = lenv_get(e, v);
        lval_del(v);
        return x;
    }
    /* evaluate s expression*/
    if (v->type == LVAL_SEXPRE){
        return lval_eval_sexpre(e, v);
    }

    return v;
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func){
    lval* k = lval_sym(name);
    lval* v = lval_fun(func);
    lenv_put(e, k, v);
    lval_del(k); 
    lval_del(v);
}

void lenv_add_builtins(lenv* e){
    /* built-in list functions */
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "first", builtin_first);
    lenv_add_builtin(e, "last", builtin_last);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);

    /* built-in math functions */
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);

    /* Variable definition */
    lenv_add_builtin(e, "def", builtin_def);
}

int main(int argc, char** argv) {
    
    /* initial parsers */
    mpc_parser_t* Number      = mpc_new("number");
    mpc_parser_t* Symbol      = mpc_new("symbol");
    mpc_parser_t* Sexpression = mpc_new("sexpression");
    mpc_parser_t* Qexpression = mpc_new("qexpression");
    mpc_parser_t* Expression  = mpc_new("expression");
    mpc_parser_t* Phrase      = mpc_new("phrase");
    
    /* Defined as follows */
    mpca_lang(MPCA_LANG_DEFAULT,
              "                                                                                 \
              number     : /-?[0-9]+/ ;                                                         \
              symbol     : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/;                                    \
              sexpression: '(' <expression>* ')' ;                                              \
              qexpression: '{' <expression>* '}' ;                                              \
              expression : <number> | <symbol> | <sexpression> | <qexpression> ;                \
              phrase     : /^/ <expression>* /$/ ;                                              \
              ",
              Number, Symbol, Sexpression, Qexpression, Expression, Phrase);
    
    /* Print Version and Exit info */
    puts("NnamLISP Version  0.0.0.5");
    puts("Press CTRL+C to Exit \n");
    lenv* e = lenv_new();
    lenv_add_builtins(e);
    
    while(1){
        char* input = readline("NnamLISP> ");
        
        /* adding command to history */
        add_history(input);
        
        
        /* attempt to parse user input */
        mpc_result_t r;
        
        if (mpc_parse("<stdin>", input, Phrase, &r)){
            lval* x = lval_eval(e, lval_read(r.output));
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        }else{
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        
        free(input);
    }
    lenv_del(e);

    mpc_cleanup(6, Number, Symbol, Sexpression, Qexpression, Expression, Phrase);
    
    return 0;
}
