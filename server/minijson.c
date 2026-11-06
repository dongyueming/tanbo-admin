#include "minijson.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>

/* ---------------------------------------------------------------- helpers */

static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) { fprintf(stderr, "out of memory\n"); exit(1); }
    return p;
}

static void *xrealloc(void *old, size_t n) {
    void *p = realloc(old, n ? n : 1);
    if (!p) { fprintf(stderr, "out of memory\n"); exit(1); }
    return p;
}

static JVal *jnew(JType t) {
    JVal *v = (JVal *)xmalloc(sizeof(JVal));
    memset(v, 0, sizeof(JVal));
    v->type = t;
    return v;
}

JVal *jnull(void) { return jnew(J_NULL); }

JVal *jbool(int b) {
    JVal *v = jnew(J_BOOL);
    v->b = b ? 1 : 0;
    return v;
}

JVal *jnum(double n) {
    JVal *v = jnew(J_NUM);
    v->num = n;
    return v;
}

JVal *jstr(const char *s) {
    JVal *v = jnew(J_STR);
    v->slen = s ? strlen(s) : 0;
    v->s = (char *)xmalloc(v->slen + 1);
    if (s) memcpy(v->s, s, v->slen);
    v->s[v->slen] = '\0';
    return v;
}

JVal *jarr(void) { return jnew(J_ARR); }
JVal *jobj(void) { return jnew(J_OBJ); }

void jfree(JVal *v) {
    if (!v) return;
    int i;
    if (v->type == J_STR) {
        free(v->s);
    } else if (v->type == J_ARR || v->type == J_OBJ) {
        for (i = 0; i < v->count; i++) {
            jfree(v->items[i]);
            if (v->keys) free(v->keys[i]);
        }
        free(v->items);
        if (v->keys) free(v->keys);
    }
    free(v);
}

/* ---------------------------------------------------------------- growth */

static void reserve_items(JVal *v, int need) {
    if (need <= v->cap) return;
    int cap = v->cap ? v->cap : 4;
    while (cap < need) cap *= 2;
    v->items = (JVal **)xrealloc(v->items, (size_t)cap * sizeof(JVal *));
    if (v->type == J_OBJ) {
        v->keys = (char **)xrealloc(v->keys, (size_t)cap * sizeof(char *));
    }
    v->cap = cap;
}

/* ---------------------------------------------------------------- parse */

typedef struct {
    const char *p;
    size_t n;
    size_t i;
    int err;
} P;

static JVal *parse_value(P *ps);

static void skip_ws(P *ps) {
    while (ps->i < ps->n) {
        char c = ps->p[ps->i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') ps->i++;
        else break;
    }
}

static void utf8_put(char **buf, size_t *len, size_t *cap, unsigned int cp) {
    char enc[4];
    int n;
    if (cp < 0x80) {
        enc[0] = (char)cp; n = 1;
    } else if (cp < 0x800) {
        enc[0] = (char)(0xC0 | (cp >> 6));
        enc[1] = (char)(0x80 | (cp & 0x3F));
        n = 2;
    } else if (cp < 0x10000) {
        enc[0] = (char)(0xE0 | (cp >> 12));
        enc[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        enc[2] = (char)(0x80 | (cp & 0x3F));
        n = 3;
    } else {
        enc[0] = (char)(0xF0 | (cp >> 18));
        enc[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        enc[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        enc[3] = (char)(0x80 | (cp & 0x3F));
        n = 4;
    }
    if (*len + (size_t)n + 1 > *cap) {
        while (*len + (size_t)n + 1 > *cap) *cap *= 2;
        *buf = (char *)xrealloc(*buf, *cap);
    }
    memcpy(*buf + *len, enc, (size_t)n);
    *len += (size_t)n;
}

static JVal *parse_string(P *ps) {
    size_t cap = 32, len = 0;
    char *buf;
    if (ps->p[ps->i] != '"') { ps->err = 1; return NULL; }
    ps->i++;
    buf = (char *)xmalloc(cap);

    while (ps->i < ps->n) {
        char c = ps->p[ps->i];
        if (c == '"') {
            ps->i++;
            JVal *v = jnew(J_STR);
            v->s = buf;
            v->slen = len;
            if (len + 1 > cap) buf = (char *)xrealloc(buf, len + 1);
            buf[len] = '\0';
            v->s = buf;
            return v;
        }
        if (c == '\\') {
            ps->i++;
            if (ps->i >= ps->n) break;
            char e = ps->p[ps->i++];
            char out = 0;
            switch (e) {
                case '"': out = '"'; break;
                case '\\': out = '\\'; break;
                case '/': out = '/'; break;
                case 'b': out = '\b'; break;
                case 'f': out = '\f'; break;
                case 'n': out = '\n'; break;
                case 'r': out = '\r'; break;
                case 't': out = '\t'; break;
                case 'u': {
                    unsigned int cp = 0;
                    int k;
                    if (ps->i + 4 > ps->n) { ps->err = 1; free(buf); return NULL; }
                    for (k = 0; k < 4; k++) {
                        char h = ps->p[ps->i + k];
                        cp <<= 4;
                        if (h >= '0' && h <= '9') cp |= (unsigned int)(h - '0');
                        else if (h >= 'a' && h <= 'f') cp |= (unsigned int)(h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') cp |= (unsigned int)(h - 'A' + 10);
                        else { ps->err = 1; free(buf); return NULL; }
                    }
                    ps->i += 4;
                    /* surrogate pair */
                    if (cp >= 0xD800 && cp <= 0xDBFF && ps->i + 6 <= ps->n &&
                        ps->p[ps->i] == '\\' && ps->p[ps->i + 1] == 'u') {
                        unsigned int lo = 0;
                        int ok = 1;
                        for (k = 0; k < 4; k++) {
                            char h = ps->p[ps->i + 2 + k];
                            lo <<= 4;
                            if (h >= '0' && h <= '9') lo |= (unsigned int)(h - '0');
                            else if (h >= 'a' && h <= 'f') lo |= (unsigned int)(h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') lo |= (unsigned int)(h - 'A' + 10);
                            else { ok = 0; break; }
                        }
                        if (ok && lo >= 0xDC00 && lo <= 0xDFFF) {
                            ps->i += 6;
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        }
                    }
                    utf8_put(&buf, &len, &cap, cp);
                    out = 0;
                    break;
                }
                default: ps->err = 1; free(buf); return NULL;
            }
            if (e == 'u') continue;
            if (out) {
                if (len + 2 > cap) { cap *= 2; buf = (char *)xrealloc(buf, cap); }
                buf[len++] = out;
            }
            continue;
        }
        if (len + 2 > cap) { cap *= 2; buf = (char *)xrealloc(buf, cap); }
        buf[len++] = c;
        ps->i++;
    }
    ps->err = 1;
    free(buf);
    return NULL;
}

static JVal *parse_number(P *ps) {
    char tmp[64];
    size_t start = ps->i, k = 0;
    if (ps->i < ps->n && (ps->p[ps->i] == '-' || ps->p[ps->i] == '+')) ps->i++;
    while (ps->i < ps->n) {
        char c = ps->p[ps->i];
        if ((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-') {
            ps->i++;
        } else break;
    }
    if (ps->i == start) { ps->err = 1; return NULL; }
    k = ps->i - start;
    if (k >= sizeof(tmp)) k = sizeof(tmp) - 1;
    memcpy(tmp, ps->p + start, k);
    tmp[k] = '\0';
    return jnum(strtod(tmp, NULL));
}

static JVal *parse_array(P *ps) {
    JVal *arr = jarr();
    ps->i++; /* [ */
    skip_ws(ps);
    if (ps->i < ps->n && ps->p[ps->i] == ']') { ps->i++; return arr; }
    while (ps->i < ps->n) {
        JVal *item;
        skip_ws(ps);
        item = parse_value(ps);
        if (!item) { jfree(arr); return NULL; }
        jpush(arr, item);
        skip_ws(ps);
        if (ps->i < ps->n && ps->p[ps->i] == ',') { ps->i++; continue; }
        if (ps->i < ps->n && ps->p[ps->i] == ']') { ps->i++; return arr; }
        ps->err = 1;
        jfree(arr);
        return NULL;
    }
    ps->err = 1;
    jfree(arr);
    return NULL;
}

static JVal *parse_object(P *ps) {
    JVal *obj = jobj();
    ps->i++; /* { */
    skip_ws(ps);
    if (ps->i < ps->n && ps->p[ps->i] == '}') { ps->i++; return obj; }
    while (ps->i < ps->n) {
        JVal *key;
        JVal *val;
        skip_ws(ps);
        key = parse_string(ps);
        if (!key) { jfree(obj); return NULL; }
        skip_ws(ps);
        if (ps->i >= ps->n || ps->p[ps->i] != ':') {
            ps->err = 1; jfree(key); jfree(obj); return NULL;
        }
        ps->i++;
        skip_ws(ps);
        val = parse_value(ps);
        if (!val) { jfree(key); jfree(obj); return NULL; }
        jset(obj, key->s, val);
        jfree(key);
        skip_ws(ps);
        if (ps->i < ps->n && ps->p[ps->i] == ',') { ps->i++; continue; }
        if (ps->i < ps->n && ps->p[ps->i] == '}') { ps->i++; return obj; }
        ps->err = 1;
        jfree(obj);
        return NULL;
    }
    ps->err = 1;
    jfree(obj);
    return NULL;
}

static JVal *parse_value(P *ps) {
    skip_ws(ps);
    if (ps->i >= ps->n) { ps->err = 1; return NULL; }
    char c = ps->p[ps->i];
    if (c == '"') return parse_string(ps);
    if (c == '{') return parse_object(ps);
    if (c == '[') return parse_array(ps);
    if (c == 't' && ps->i + 3 < ps->n && memcmp(ps->p + ps->i, "true", 4) == 0) {
        ps->i += 4;
        return jbool(1);
    }
    if (c == 'f' && ps->i + 4 < ps->n && memcmp(ps->p + ps->i, "false", 5) == 0) {
        ps->i += 5;
        return jbool(0);
    }
    if (c == 'n' && ps->i + 3 < ps->n && memcmp(ps->p + ps->i, "null", 4) == 0) {
        ps->i += 4;
        return jnull();
    }
    if (c == '-' || c == '+' || (c >= '0' && c <= '9')) return parse_number(ps);
    ps->err = 1;
    return NULL;
}

JVal *jparse(const char *text, size_t len) {
    P ps;
    JVal *v;
    ps.p = text;
    ps.n = len;
    ps.i = 0;
    ps.err = 0;
    v = parse_value(&ps);
    if (ps.err || !v) {
        jfree(v);
        return NULL;
    }
    return v;
}

/* ---------------------------------------------------------------- access */

JVal *jget(const JVal *obj, const char *key) {
    int i;
    if (!obj || obj->type != J_OBJ) return NULL;
    for (i = 0; i < obj->count; i++) {
        if (strcmp(obj->keys[i], key) == 0) return obj->items[i];
    }
    return NULL;
}

JVal *jidx(const JVal *v, int i) {
    if (!v || (v->type != J_ARR && v->type != J_OBJ)) return NULL;
    if (i < 0 || i >= v->count) return NULL;
    return v->items[i];
}

const char *jkey(const JVal *v, int i) {
    if (!v || v->type != J_OBJ || !v->keys) return NULL;
    if (i < 0 || i >= v->count) return NULL;
    return v->keys[i];
}

int jlen(const JVal *v) {
    if (!v) return 0;
    if (v->type == J_ARR || v->type == J_OBJ) return v->count;
    return 0;
}

void jpush(JVal *arr, JVal *item) {
    if (!arr || arr->type != J_ARR) return;
    reserve_items(arr, arr->count + 1);
    arr->items[arr->count++] = item;
}

void jset(JVal *obj, const char *key, JVal *val) {
    int i;
    if (!obj || obj->type != J_OBJ) return;
    for (i = 0; i < obj->count; i++) {
        if (strcmp(obj->keys[i], key) == 0) {
            jfree(obj->items[i]);
            obj->items[i] = val;
            return;
        }
    }
    reserve_items(obj, obj->count + 1);
    obj->items[obj->count] = val;
    obj->keys[obj->count] = (char *)xmalloc(strlen(key) + 1);
    strcpy(obj->keys[obj->count], key);
    obj->count++;
}

void jdel(JVal *obj, const char *key) {
    int i;
    if (!obj || obj->type != J_OBJ) return;
    for (i = 0; i < obj->count; i++) {
        if (strcmp(obj->keys[i], key) == 0) {
            int j;
            jfree(obj->items[i]);
            free(obj->keys[i]);
            for (j = i; j + 1 < obj->count; j++) {
                obj->items[j] = obj->items[j + 1];
                obj->keys[j] = obj->keys[j + 1];
            }
            obj->count--;
            return;
        }
    }
}

double jnumv(const JVal *v) {
    if (!v) return 0;
    if (v->type == J_NUM) return v->num;
    if (v->type == J_BOOL) return v->b ? 1 : 0;
    if (v->type == J_STR) return strtod(v->s, NULL);
    return 0;
}

const char *jstrv(const JVal *v) {
    if (!v) return NULL;
    if (v->type == J_STR) return v->s;
    return NULL;
}

int jboolv(const JVal *v) {
    if (!v) return 0;
    if (v->type == J_BOOL) return v->b;
    if (v->type == J_NUM) return v->num != 0;
    return 0;
}

/* ---------------------------------------------------------------- buffer */

void jbuf_init(JBuf *b) {
    b->cap = 1024;
    b->len = 0;
    b->data = (char *)xmalloc(b->cap);
    b->data[0] = '\0';
}

void jbuf_free(JBuf *b) {
    free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
}

static void jbuf_reserve(JBuf *b, size_t need) {
    if (b->len + need + 1 <= b->cap) return;
    while (b->len + need + 1 > b->cap) b->cap *= 2;
    b->data = (char *)xrealloc(b->data, b->cap);
}

void jbuf_add(JBuf *b, const char *s) {
    size_t n = strlen(s);
    jbuf_reserve(b, n);
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

void jbuf_printf(JBuf *b, const char *fmt, ...) {
    va_list ap;
    char tmp[512];
    int n;
    va_start(ap, fmt);
    n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if ((size_t)n >= sizeof(tmp)) n = (int)sizeof(tmp) - 1;
    jbuf_reserve(b, (size_t)n);
    memcpy(b->data + b->len, tmp, (size_t)n);
    b->len += (size_t)n;
    b->data[b->len] = '\0';
}

static void ser_string(const char *s, size_t len, JBuf *b) {
    size_t i;
    jbuf_add(b, "\"");
    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
            case '"': jbuf_add(b, "\\\""); break;
            case '\\': jbuf_add(b, "\\\\"); break;
            case '\n': jbuf_add(b, "\\n"); break;
            case '\r': jbuf_add(b, "\\r"); break;
            case '\t': jbuf_add(b, "\\t"); break;
            case '\b': jbuf_add(b, "\\b"); break;
            case '\f': jbuf_add(b, "\\f"); break;
            default:
                if (c < 0x20) jbuf_printf(b, "\\u%04x", (unsigned int)c);
                else {
                    char one[2];
                    one[0] = (char)c;
                    one[1] = '\0';
                    jbuf_add(b, one);
                }
        }
    }
    jbuf_add(b, "\"");
}

static void ser_val(const JVal *v, JBuf *b) {
    int i;
    if (!v) { jbuf_add(b, "null"); return; }
    switch (v->type) {
        case J_NULL:
            jbuf_add(b, "null");
            break;
        case J_BOOL:
            jbuf_add(b, v->b ? "true" : "false");
            break;
        case J_NUM: {
            double d = v->num;
            if (d == 0) { jbuf_add(b, "0"); break; }
            if (fabs(d) < 1e15 && d == (double)(long long)d) {
                jbuf_printf(b, "%lld", (long long)d);
            } else {
                char tmp[64];
                snprintf(tmp, sizeof(tmp), "%.10g", d);
                jbuf_add(b, tmp);
            }
            break;
        }
        case J_STR:
            ser_string(v->s ? v->s : "", v->slen, b);
            break;
        case J_ARR:
            jbuf_add(b, "[");
            for (i = 0; i < v->count; i++) {
                if (i) jbuf_add(b, ",");
                ser_val(v->items[i], b);
            }
            jbuf_add(b, "]");
            break;
        case J_OBJ:
            jbuf_add(b, "{");
            for (i = 0; i < v->count; i++) {
                if (i) jbuf_add(b, ",");
                ser_string(v->keys[i], strlen(v->keys[i]), b);
                jbuf_add(b, ":");
                ser_val(v->items[i], b);
            }
            jbuf_add(b, "}");
            break;
    }
}

void jserialize(const JVal *v, JBuf *out) {
    ser_val(v, out);
}
