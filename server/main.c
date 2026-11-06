#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <direct.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET sock_t;
#define CLOSESOCK closesocket
#define SOCK_INVALID INVALID_SOCKET
#define SOCK_ERRNO WSAGetLastError()
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
typedef int sock_t;
#define CLOSESOCK close
#define SOCK_INVALID (-1)
#define SOCK_ERRNO errno
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#define str_icmp_n _strnicmp
#else
#include <strings.h>
#define str_icmp_n strncasecmp
#endif

#include "minijson.h"

#define MAX_PATH_LEN 1024
#define MAX_HEADER_BYTES 16384
#define MAX_BODY_BYTES (4 * 1024 * 1024)

/* ---------------------------------------------------------------- config */

static int g_port = 8080;
static char g_data_dir[MAX_PATH_LEN] = "data";
static char g_static_dir[MAX_PATH_LEN] = "dist";

/* 登录账号：手机号 + 固定密码，来自 data/accounts.txt，格式  手机号,密码 */
typedef struct {
    char phone[32];
    char pass[64];
} Account;

static Account g_accounts[8];
static int g_account_n = 0;

#define MAX_SESSIONS 64
static char g_sess_token[MAX_SESSIONS][40];
static char g_sess_phone[MAX_SESSIONS][32];
static int g_sess_n = 0;

/* ---------------------------------------------------------------- stores */

static const char *TYPES[] = { "batches", "strands", "sales", "losses", "expenses" };
#define NTYPES 5

static JVal *g_store[NTYPES];
static int g_loaded[NTYPES];

static int type_index(const char *name) {
    int i;
    for (i = 0; i < NTYPES; i++) {
        if (strcmp(TYPES[i], name) == 0) return i;
    }
    return -1;
}

static int make_dir(const char *p) {
#ifdef _WIN32
    return _mkdir(p);
#else
    return mkdir(p, 0755);
#endif
}

static int read_file(const char *path, char **out, size_t *outlen) {
    FILE *f = fopen(path, "rb");
    long n;
    char *buf;
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    if (n < 0) { fclose(f); return 0; }
    fseek(f, 0, SEEK_SET);
    buf = (char *)malloc((size_t)n + 1);
    if (!buf) { fclose(f); return 0; }
    if (n > 0 && fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf);
        fclose(f);
        return 0;
    }
    buf[n] = '\0';
    fclose(f);
    *out = buf;
    *outlen = (size_t)n;
    return 1;
}

static int file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

/* ------------------------------------------------ accounts & sessions */

static void trim_eol(char *s) {
    size_t n = strlen(s);
    while (n > 0) {
        char c = s[n - 1];
        if (c == '\r' || c == '\n' || c == ' ' || c == '\t') s[--n] = '\0';
        else break;
    }
}

static void load_accounts(void) {
    char path[MAX_PATH_LEN];
    FILE *f;
    char line[160];

    g_account_n = 0;
    snprintf(path, sizeof(path), "%s/accounts.txt", g_data_dir);
    f = fopen(path, "rb");
    if (!f) return;
    while (fgets(line, sizeof(line), f)) {
        char *comma;
        size_t i;
        if (g_account_n >= (int)(sizeof(g_accounts) / sizeof(g_accounts[0]))) break;
        trim_eol(line);
        if (line[0] == '\0' || line[0] == '#') continue;
        comma = strchr(line, ',');
        if (!comma) continue;
        *comma = '\0';
        trim_eol(line);
        for (i = 0; line[i]; i++) { /* 去掉手机号里的空格和横线 */ }
        snprintf(g_accounts[g_account_n].phone, sizeof(g_accounts[g_account_n].phone), "%s", line);
        snprintf(g_accounts[g_account_n].pass, sizeof(g_accounts[g_account_n].pass), "%s", comma + 1);
        trim_eol(g_accounts[g_account_n].pass);
        g_account_n++;
    }
    fclose(f);
}

static int find_account(const char *phone, const char *pass) {
    int i;
    for (i = 0; i < g_account_n; i++) {
        if (strcmp(g_accounts[i].phone, phone) == 0 && strcmp(g_accounts[i].pass, pass) == 0) return i;
    }
    return -1;
}

static int phone_allowed(const char *phone) {
    int i;
    for (i = 0; i < g_account_n; i++) {
        if (strcmp(g_accounts[i].phone, phone) == 0) return 1;
    }
    return 0;
}

static void new_token(char *out, int len) {
    static const char *hex = "0123456789abcdef";
    int i;
    for (i = 0; i < len; i++) out[i] = hex[rand() % 16];
    out[len] = '\0';
}

static const char *session_owner(const char *token) {
    int i;
    if (!token || !token[0]) return NULL;
    for (i = 0; i < g_sess_n; i++) {
        if (strcmp(g_sess_token[i], token) == 0) return g_sess_phone[i];
    }
    return NULL;
}

static int drop_session(const char *token) {
    int i;
    for (i = 0; i < g_sess_n; i++) {
        if (strcmp(g_sess_token[i], token) == 0) {
            g_sess_token[i][0] = '\0';
            g_sess_phone[i][0] = '\0';
            return 1;
        }
    }
    return 0;
}

static const char *cookie_value(const char *cookie, const char *name) {
    static char buf[64];
    const char *p = cookie;
    size_t nlen = strlen(name);

    while (p && *p) {
        while (*p == ' ' || *p == ';') p++;
        if (_strnicmp(p, name, nlen) == 0 && p[nlen] == '=') {
            const char *v = p + nlen + 1;
            const char *e = v;
            size_t n;
            while (*e && *e != ';') e++;
            n = (size_t)(e - v);
            if (n >= sizeof(buf)) n = sizeof(buf) - 1;
            memcpy(buf, v, n);
            buf[n] = '\0';
            return buf;
        }
        p = strchr(p, ';');
    }
    return NULL;
}

static void load_store(int i) {
    char path[MAX_PATH_LEN];
    char *txt = NULL;
    size_t len = 0;

    if (g_loaded[i]) return;
    snprintf(path, sizeof(path), "%s/%s.json", g_data_dir, TYPES[i]);
    if (read_file(path, &txt, &len)) {
        JVal *v = jparse(txt, len);
        if (v && v->type == J_ARR) {
            g_store[i] = v;
        } else {
            jfree(v);
            g_store[i] = jarr();
        }
        free(txt);
    } else {
        g_store[i] = jarr();
    }
    g_loaded[i] = 1;
}

static void save_store(int i) {
    char path[MAX_PATH_LEN];
    char tmp[MAX_PATH_LEN];
    JBuf buf;
    FILE *f;

    snprintf(path, sizeof(path), "%s/%s.json", g_data_dir, TYPES[i]);
    snprintf(tmp, sizeof(tmp), "%s/%s.json.tmp", g_data_dir, TYPES[i]);

    jbuf_init(&buf);
    jserialize(g_store[i], &buf);

    f = fopen(tmp, "wb");
    if (f) {
        if (buf.len) fwrite(buf.data, 1, buf.len, f);
        fclose(f);
        remove(path);
        rename(tmp, path);
    }
    jbuf_free(&buf);
}

/* ---------------------------------------------------------------- helpers */

static double next_id(JVal *list) {
    double mx = 0;
    int i;
    for (i = 0; i < jlen(list); i++) {
        double id = jnumv(jget(jidx(list, i), "id"));
        if (id > mx) mx = id;
    }
    return mx + 1;
}

static JVal *find_by_id(JVal *list, double id) {
    int i;
    for (i = 0; i < jlen(list); i++) {
        JVal *it = jidx(list, i);
        if (jnumv(jget(it, "id")) == id) return it;
    }
    return NULL;
}

static int remove_by_id(JVal *list, double id) {
    int i;
    for (i = 0; i < jlen(list); i++) {
        JVal *it = jidx(list, i);
        if (jnumv(jget(it, "id")) == id) {
            int j;
            jfree(list->items[i]);
            for (j = i; j + 1 < list->count; j++) list->items[j] = list->items[j + 1];
            list->count--;
            return 1;
        }
    }
    return 0;
}

/* merge src object keys into dst (skipping "id") */
static void merge_into(JVal *dst, JVal *src) {
    int i;
    for (i = 0; i < jlen(src); i++) {
        const char *k = jkey(src, i);
        if (!k) continue;
        if (strcmp(k, "id") == 0) continue;
        jset(dst, k, jidx(src, i));
        src->items[i] = NULL; /* ownership moved */
    }
    /* rebuild src so jfree does not double free */
    for (i = 0; i < src->count; i++) {
        if (src->items[i] == NULL) src->items[i] = jnull();
    }
}

static const char *mime_for(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";
    if (strcmp(dot, ".html") == 0) return "text/html; charset=utf-8";
    if (strcmp(dot, ".js") == 0) return "application/javascript; charset=utf-8";
    if (strcmp(dot, ".mjs") == 0) return "application/javascript; charset=utf-8";
    if (strcmp(dot, ".css") == 0) return "text/css; charset=utf-8";
    if (strcmp(dot, ".json") == 0) return "application/json; charset=utf-8";
    if (strcmp(dot, ".svg") == 0) return "image/svg+xml";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".jpg") == 0) return "image/jpeg";
    if (strcmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(dot, ".gif") == 0) return "image/gif";
    if (strcmp(dot, ".webp") == 0) return "image/webp";
    if (strcmp(dot, ".ico") == 0) return "image/x-icon";
    if (strcmp(dot, ".woff2") == 0) return "font/woff2";
    if (strcmp(dot, ".woff") == 0) return "font/woff";
    if (strcmp(dot, ".ttf") == 0) return "font/ttf";
    if (strcmp(dot, ".txt") == 0) return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

/* ---------------------------------------------------------------- socket io */

static void send_all(sock_t s, const char *data, size_t len) {
    size_t off = 0;
    while (off < len) {
        int n = (int)send(s, data + off, (int)(len - off), 0);
        if (n <= 0) return;
        off += (size_t)n;
    }
}

static void send_response(sock_t s, int status, const char *ctype, const void *body, size_t len) {
    char head[512];
    const char *reason = "OK";
    if (status == 201) reason = "Created";
    else if (status == 204) reason = "No Content";
    else if (status == 400) reason = "Bad Request";
    else if (status == 404) reason = "Not Found";
    else if (status == 405) reason = "Method Not Allowed";
    else if (status == 413) reason = "Payload Too Large";
    else if (status == 500) reason = "Internal Server Error";

    snprintf(head, sizeof(head),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %lu\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
             "Cache-Control: no-store\r\n"
             "Connection: close\r\n"
             "\r\n",
             status, reason, ctype, (unsigned long)len);
    send_all(s, head, strlen(head));
    if (len && body) send_all(s, (const char *)body, len);
}

static void send_json(sock_t s, int status, JVal *v) {
    JBuf b;
    jbuf_init(&b);
    jserialize(v, &b);
    send_response(s, status, "application/json; charset=utf-8", b.data, b.len);
    jbuf_free(&b);
}

static void send_json_err(sock_t s, int status, const char *msg) {
    JVal *o = jobj();
    jset(o, "error", jstr(msg));
    send_json(s, status, o);
    jfree(o);
}

static void send_unauthorized(sock_t s) {
    char head[512];
    const char *body = "Authentication required";
    snprintf(head, sizeof(head),
             "HTTP/1.1 401 Unauthorized\r\n"
             "Content-Type: text/plain; charset=utf-8\r\n"
             "Content-Length: %lu\r\n"
             "WWW-Authenticate: Basic realm=\"Tanbo Admin\", charset=\"UTF-8\"\r\n"
             "Cache-Control: no-store\r\n"
             "Connection: close\r\n"
             "\r\n",
             (unsigned long)strlen(body));
    send_all(s, head, strlen(head));
    send_all(s, body, strlen(body));
}

/* 带 Set-Cookie 的 JSON 响应 */
static void send_json_with_cookie(sock_t s, int status, JVal *v, const char *set_cookie) {
    JBuf b;
    char head[1024];
    const char *reason = "OK";

    if (status == 200) reason = "OK";
    else if (status == 201) reason = "Created";
    else if (status == 400) reason = "Bad Request";
    else if (status == 401) reason = "Unauthorized";
    else if (status == 403) reason = "Forbidden";

    jbuf_init(&b);
    jserialize(v, &b);
    snprintf(head, sizeof(head),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: application/json; charset=utf-8\r\n"
             "Content-Length: %lu\r\n"
             "%s%s%s"
             "Cache-Control: no-store\r\n"
             "Connection: close\r\n"
             "\r\n",
             status, reason, (unsigned long)b.len,
             set_cookie ? "Set-Cookie: " : "",
             set_cookie ? set_cookie : "",
             set_cookie ? "\r\n" : "");
    send_all(s, head, strlen(head));
    if (b.len) send_all(s, b.data, b.len);
    jbuf_free(&b);
}

/* ---------------------------------------------------------------- api */

static void api_state(sock_t s) {
    JVal *o = jobj();
    int i;
    for (i = 0; i < NTYPES; i++) {
        load_store(i);
        jset(o, TYPES[i], g_store[i]);
    }
    send_json(s, 200, o);
    /* detach lists so freeing the envelope does not destroy the stores */
    for (i = 0; i < NTYPES; i++) o->items[i] = jnull();
    jfree(o);
}

static void api_create(sock_t s, int ti, JVal *body) {
    JVal *item;
    double id;
    if (!body || body->type != J_OBJ) {
        send_json_err(s, 400, "body must be a json object");
        return;
    }
    load_store(ti);
    id = next_id(g_store[ti]);
    jset(body, "id", jnum(id));
    jpush(g_store[ti], body);
    save_store(ti);
    send_json(s, 201, body);
}

static void api_update(sock_t s, int ti, double id, JVal *body) {
    JVal *item;
    if (!body || body->type != J_OBJ) {
        send_json_err(s, 400, "body must be a json object");
        return;
    }
    load_store(ti);
    item = find_by_id(g_store[ti], id);
    if (!item) {
        send_json_err(s, 404, "not found");
        return;
    }
    merge_into(item, body);
    save_store(ti);
    send_json(s, 200, item);
}

static void api_delete(sock_t s, int ti, double id) {
    load_store(ti);
    if (!remove_by_id(g_store[ti], id)) {
        send_json_err(s, 404, "not found");
        return;
    }
    save_store(ti);
    send_response(s, 204, "application/json", NULL, 0);
}

/* ---------------------------------------------------------------- static */

static void url_decode(char *s) {
    char *p = s;
    while (*p) {
        if (*p == '%' && p[1] && p[2]) {
            int h1 = p[1], h2 = p[2];
            int v;
            if (h1 >= '0' && h1 <= '9') v = (h1 - '0') << 4;
            else if (h1 >= 'a' && h1 <= 'f') v = (h1 - 'a' + 10) << 4;
            else if (h1 >= 'A' && h1 <= 'F') v = (h1 - 'A' + 10) << 4;
            else { p++; continue; }
            if (h2 >= '0' && h2 <= '9') v |= (h2 - '0');
            else if (h2 >= 'a' && h2 <= 'f') v |= (h2 - 'a' + 10);
            else if (h2 >= 'A' && h2 <= 'F') v |= (h2 - 'A' + 10);
            else { p++; continue; }
            *p = (char)v;
            memmove(p + 1, p + 3, strlen(p + 3) + 1);
        }
        p++;
    }
}

static void serve_file(sock_t s, const char *fs_path, const char *req_path) {
    char *data = NULL;
    size_t len = 0;
    if (read_file(fs_path, &data, &len)) {
        send_response(s, 200, mime_for(req_path), data, len);
        free(data);
    } else {
        send_response(s, 404, "text/plain; charset=utf-8", "not found", 9);
    }
}

static void serve_static(sock_t s, const char *raw_path) {
    char path[MAX_PATH_LEN];
    char fs_path[MAX_PATH_LEN];
    char *q;
    int is_root;

    snprintf(path, sizeof(path), "%s", raw_path);
    url_decode(path);
    q = strchr(path, '?');
    if (q) *q = '\0';
    q = strchr(path, '#');
    if (q) *q = '\0';

    if (strstr(path, "..")) {
        send_response(s, 400, "text/plain; charset=utf-8", "bad path", 8);
        return;
    }

    is_root = (path[0] == '\0' || strcmp(path, "/") == 0);
    if (is_root) {
        snprintf(fs_path, sizeof(fs_path), "%s/index.html", g_static_dir);
        serve_file(s, fs_path, "/index.html");
        return;
    }

    snprintf(fs_path, sizeof(fs_path), "%s%s", g_static_dir, path);
    if (file_exists(fs_path)) {
        serve_file(s, fs_path, path);
        return;
    }
    /* SPA fallback */
    snprintf(fs_path, sizeof(fs_path), "%s/index.html", g_static_dir);
    serve_file(s, fs_path, "/index.html");
}

/* ---------------------------------------------------------------- request */

typedef struct {
    char method[16];
    char path[MAX_PATH_LEN];
    char auth[256];
    char cookie[512];
    long content_length;
    char *body;
    size_t body_len;
} Request;

static int b64val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static void b64decode(const char *in, char *out, size_t outsz) {
    size_t o = 0;
    int bits = 0;
    unsigned int acc = 0;
    const char *p;
    if (outsz == 0) return;
    for (p = in; *p && o + 1 < outsz; p++) {
        int v;
        if (*p == '=') break;
        v = b64val(*p);
        if (v < 0) continue;
        acc = (acc << 6) | (unsigned int)v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out[o++] = (char)((acc >> bits) & 0xFFu);
        }
    }
    out[o] = '\0';
}

static int parse_request(const char *buf, size_t total, Request *req, const char **body_start_in_buf) {
    const char *line_end;
    const char *p;
    size_t header_len;

    memset(req, 0, sizeof(Request));
    line_end = strstr(buf, "\r\n");
    if (!line_end) return 0;
    /* METHOD SP PATH SP VERSION */
    {
        const char *sp1 = memchr(buf, ' ', (size_t)(line_end - buf));
        const char *sp2;
        if (!sp1) return 0;
        sp2 = memchr(sp1 + 1, ' ', (size_t)(line_end - sp1 - 1));
        if (!sp2) return 0;
        if ((size_t)(sp1 - buf) >= sizeof(req->method)) return 0;
        memcpy(req->method, buf, (size_t)(sp1 - buf));
        req->method[sp1 - buf] = '\0';
        if ((size_t)(sp2 - sp1 - 1) >= sizeof(req->path)) return 0;
        memcpy(req->path, sp1 + 1, (size_t)(sp2 - sp1 - 1));
        req->path[sp2 - sp1 - 1] = '\0';
    }

    p = strstr(buf, "\r\n\r\n");
    if (!p) return 0;
    header_len = (size_t)(p - buf) + 4;
    *body_start_in_buf = buf + header_len;

    /* scan headers */
    {
        const char *h = line_end + 2;
        while (h < p) {
            const char *e = strstr(h, "\r\n");
            if (!e || e > p) break;
            if ((size_t)(e - h) > 15 && _strnicmp(h, "Content-Length:", 15) == 0) {
                req->content_length = strtol(h + 15, NULL, 10);
            } else if ((size_t)(e - h) > 14 && _strnicmp(h, "Authorization:", 14) == 0) {
                const char *v = h + 14;
                while (v < e && (*v == ' ' || *v == '\t')) v++;
                if ((size_t)(e - v) < sizeof(req->auth)) {
                    memcpy(req->auth, v, (size_t)(e - v));
                    req->auth[e - v] = '\0';
                }
            } else if ((size_t)(e - h) > 7 && _strnicmp(h, "Cookie:", 7) == 0) {
                const char *v = h + 7;
                while (v < e && (*v == ' ' || *v == '\t')) v++;
                if ((size_t)(e - v) < sizeof(req->cookie)) {
                    memcpy(req->cookie, v, (size_t)(e - v));
                    req->cookie[e - v] = '\0';
                }
            }
            h = e + 2;
        }
    }
    if (req->content_length < 0) req->content_length = 0;
    req->body_len = total - header_len;
    return 1;
}

/* ---------------------------------------------------------------- router */

static void handle_api(sock_t s, Request *req) {
    const char *path = req->path;
    const char *p;

    if (strcmp(path, "/api/health") == 0 || strcmp(path, "/api/health/") == 0) {
        JVal *o = jobj();
        jset(o, "ok", jbool(1));
        jset(o, "service", jstr("tanbo-admin"));
        send_json(s, 200, o);
        jfree(o);
        return;
    }

    if (strcmp(path, "/api/login") == 0) {
        JVal *body;
        const char *phone;
        const char *pass;
        char tok[40];
        char cookie[256];
        int slot;
        JVal *o;

        if (strcmp(req->method, "POST") != 0) { send_json_err(s, 405, "method not allowed"); return; }
        body = (req->body && req->body_len) ? jparse(req->body, req->body_len) : NULL;
        phone = jstrv(jget(body, "phone"));
        pass = jstrv(jget(body, "password"));
        if (find_account(phone, pass) >= 0) {
            new_token(tok, 32);
            if (g_sess_n < MAX_SESSIONS) slot = g_sess_n++;
            else {
                int i, victim = 0;
                for (i = 0; i < MAX_SESSIONS; i++) {
                    if (g_sess_token[i][0] == '\0') { victim = i; break; }
                }
                slot = victim;
            }
            snprintf(g_sess_token[slot], sizeof(g_sess_token[slot]), "%s", tok);
            snprintf(g_sess_phone[slot], sizeof(g_sess_phone[slot]), "%s", phone);
            snprintf(cookie, sizeof(cookie), "sid=%s; Path=/; HttpOnly; SameSite=Lax; Max-Age=604800", tok);
            o = jobj();
            jset(o, "ok", jbool(1));
            jset(o, "phone", jstr(phone));
            send_json_with_cookie(s, 200, o, cookie);
            jfree(o);
        } else if (phone_allowed(phone)) {
            send_json_err(s, 401, "密码不对");
        } else {
            send_json_err(s, 403, "这个手机号没有登录权限");
        }
        jfree(body);
        return;
    }

    if (strcmp(path, "/api/logout") == 0) {
        JVal *o = jobj();
        const char *tok = cookie_value(req->cookie, "sid");
        if (tok) drop_session(tok);
        jset(o, "ok", jbool(1));
        send_json_with_cookie(s, 200, o, "sid=; Path=/; Max-Age=0");
        jfree(o);
        return;
    }

    if (strcmp(path, "/api/me") == 0) {
        const char *owner = session_owner(cookie_value(req->cookie, "sid"));
        JVal *o;
        if (!owner) { send_json_err(s, 401, "未登录"); return; }
        o = jobj();
        jset(o, "ok", jbool(1));
        jset(o, "phone", jstr(owner));
        send_json(s, 200, o);
        jfree(o);
        return;
    }

    if (strcmp(path, "/api/state") == 0) {
        if (strcmp(req->method, "GET") != 0) { send_json_err(s, 405, "method not allowed"); return; }
        api_state(s);
        return;
    }

    if (strncmp(path, "/api/items/", 11) != 0) {
        send_json_err(s, 404, "unknown api");
        return;
    }

    p = path + 11;
    {
        char type_name[64];
        const char *slash = strchr(p, '/');
        int ti;
        double id = 0;

        if (slash) {
            size_t n = (size_t)(slash - p);
            if (n >= sizeof(type_name)) n = sizeof(type_name) - 1;
            memcpy(type_name, p, n);
            type_name[n] = '\0';
            id = strtod(slash + 1, NULL);
        } else {
            snprintf(type_name, sizeof(type_name), "%s", p);
        }
        ti = type_index(type_name);
        if (ti < 0) { send_json_err(s, 404, "unknown collection"); return; }

        if (strcmp(req->method, "POST") == 0 && !slash) {
            JVal *body = req->body && req->body_len ? jparse(req->body, req->body_len) : NULL;
            api_create(s, ti, body);
            return;
        }
        if (strcmp(req->method, "PUT") == 0 && slash) {
            JVal *body = req->body && req->body_len ? jparse(req->body, req->body_len) : NULL;
            api_update(s, ti, id, body);
            return;
        }
        if (strcmp(req->method, "DELETE") == 0 && slash) {
            api_delete(s, ti, id);
            return;
        }
        if (strcmp(req->method, "GET") == 0 && !slash) {
            load_store(ti);
            send_json(s, 200, g_store[ti]);
            return;
        }
        send_json_err(s, 405, "method not allowed");
    }
}

static void handle_client(sock_t c) {
    char *buf;
    size_t total = 0;
    Request req;
    const char *body_in_buf = NULL;
    int header_done = 0;

    buf = (char *)malloc(MAX_HEADER_BYTES + MAX_BODY_BYTES + 1);
    if (!buf) { CLOSESOCK(c); return; }

    while (total < MAX_HEADER_BYTES) {
        int n = (int)recv(c, buf + total, (int)(MAX_HEADER_BYTES - total), 0);
        if (n <= 0) { free(buf); CLOSESOCK(c); return; }
        total += (size_t)n;
        buf[total] = '\0';
        if (strstr(buf, "\r\n\r\n")) { header_done = 1; break; }
    }

    if (!header_done) { free(buf); CLOSESOCK(c); return; }

    if (!parse_request(buf, total, &req, &body_in_buf)) {
        send_response(c, 400, "text/plain; charset=utf-8", "bad request", 11);
        free(buf);
        CLOSESOCK(c);
        return;
    }

    if (req.content_length > MAX_BODY_BYTES) {
        send_response(c, 413, "text/plain; charset=utf-8", "too large", 9);
        free(buf);
        CLOSESOCK(c);
        return;
    }

    /* make sure whole body is buffered */
    if ((long)(total - (size_t)(body_in_buf - buf)) < req.content_length) {
        size_t have = total - (size_t)(body_in_buf - buf);
        while ((long)have < req.content_length) {
            int n;
            if (total >= MAX_HEADER_BYTES + MAX_BODY_BYTES) break;
            n = (int)recv(c, buf + total, (int)(MAX_HEADER_BYTES + MAX_BODY_BYTES - total), 0);
            if (n <= 0) break;
            total += (size_t)n;
            have += (size_t)n;
            buf[total] = '\0';
        }
    }
    req.body = (char *)body_in_buf;    req.body_len = (size_t)req.content_length;

    /* 访问控制：只保护 /api/ 下的数据接口；页面骨架（静态资源）放行，由前端显示登录页 */
    if (strncmp(req.path, "/api/", 5) == 0) {
        int open_path = strcmp(req.path, "/api/health") == 0 ||
                        strcmp(req.path, "/api/login") == 0 ||
                        strcmp(req.path, "/api/logout") == 0;
        if (!open_path && strcmp(req.method, "OPTIONS") != 0) {
            const char *owner = session_owner(cookie_value(req.cookie, "sid"));
            if (!owner) {
                send_json_err(c, 401, "未登录");
                free(buf);
                CLOSESOCK(c);
                return;
            }
        }
    }

    if (strcmp(req.method, "OPTIONS") == 0) {
        send_response(c, 204, "text/plain", NULL, 0);
    } else if (strncmp(req.path, "/api/", 5) == 0) {
        handle_api(c, &req);
    } else if (strcmp(req.method, "GET") == 0 || strcmp(req.method, "HEAD") == 0) {
        serve_static(c, req.path);
    } else {
        send_response(c, 405, "text/plain; charset=utf-8", "method not allowed", 18);
    }

    free(buf);
    CLOSESOCK(c);
}

/* ---------------------------------------------------------------- main */

int main(int argc, char **argv) {
    int i;
    sock_t listen_fd;
    struct sockaddr_in addr;
    int opt = 1;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            g_port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--data") == 0 && i + 1 < argc) {
            snprintf(g_data_dir, sizeof(g_data_dir), "%s", argv[++i]);
        } else if (strcmp(argv[i], "--static") == 0 && i + 1 < argc) {
            snprintf(g_static_dir, sizeof(g_static_dir), "%s", argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("usage: tanbo-server [--port N] [--data DIR] [--static DIR]\n");
            printf("accounts come from <data>/accounts.txt, one 'phone,password' per line\n");
            return 0;
        }
    }

    if (getenv("PORT")) g_port = atoi(getenv("PORT"));
    if (getenv("DATA_DIR")) snprintf(g_data_dir, sizeof(g_data_dir), "%s", getenv("DATA_DIR"));
    if (getenv("STATIC_DIR")) snprintf(g_static_dir, sizeof(g_static_dir), "%s", getenv("STATIC_DIR"));

    srand((unsigned int)(time(NULL) ^ (unsigned long)clock()));
    make_dir(g_data_dir);
    load_accounts();

#ifdef _WIN32
    {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            fprintf(stderr, "WSAStartup failed\n");
            return 1;
        }
    }
#endif

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == SOCK_INVALID) {
        fprintf(stderr, "socket() failed: %d\n", SOCK_ERRNO);
        return 1;
    }
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)g_port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        fprintf(stderr, "bind() failed on port %d: %d\n", g_port, SOCK_ERRNO);
        CLOSESOCK(listen_fd);
        return 1;
    }
    if (listen(listen_fd, 64) != 0) {
        fprintf(stderr, "listen() failed: %d\n", SOCK_ERRNO);
        CLOSESOCK(listen_fd);
        return 1;
    }

    printf("tanbo-server listening on http://0.0.0.0:%d\n", g_port);
    printf("  data dir : %s\n", g_data_dir);
    printf("  static   : %s\n", g_static_dir);
    fflush(stdout);

    while (1) {
        sock_t c = accept(listen_fd, NULL, NULL);
        if (c == SOCK_INVALID) continue;
        handle_client(c);
    }

    CLOSESOCK(listen_fd);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
