#ifndef MINIJSON_H
#define MINIJSON_H

#include <stddef.h>

typedef enum {
    J_NULL = 0,
    J_BOOL,
    J_NUM,
    J_STR,
    J_ARR,
    J_OBJ
} JType;

typedef struct JVal {
    JType type;
    int b;
    double num;
    char *s;
    size_t slen;
    struct JVal **items;
    char **keys;
    int count;
    int cap;
} JVal;

/* constructors */
JVal *jnull(void);
JVal *jbool(int v);
JVal *jnum(double v);
JVal *jstr(const char *s);
JVal *jarr(void);
JVal *jobj(void);
void jfree(JVal *v);

/* parse a JSON document, returns NULL on error */
JVal *jparse(const char *text, size_t len);

/* navigation */
JVal *jget(const JVal *obj, const char *key);
JVal *jidx(const JVal *v, int i);
const char *jkey(const JVal *v, int i);
int jlen(const JVal *v);

/* mutation */
void jpush(JVal *arr, JVal *item);
void jset(JVal *obj, const char *key, JVal *val);
void jdel(JVal *obj, const char *key);

/* accessors (lenient) */
double jnumv(const JVal *v);
const char *jstrv(const JVal *v);
int jboolv(const JVal *v);

/* output buffer */
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} JBuf;

void jbuf_init(JBuf *b);
void jbuf_free(JBuf *b);
void jbuf_add(JBuf *b, const char *s);
void jbuf_printf(JBuf *b, const char *fmt, ...);
void jserialize(const JVal *v, JBuf *out);

#endif
