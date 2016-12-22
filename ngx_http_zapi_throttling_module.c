//
// Created by Masudur Rahman on 12/17/16.
//


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <mysql.h>
#include <time.h>


#define ZAPI_THROTTLING_MODULE_VERSION "0.1"

#define NGXCSTR(s) ((s).data ? strdup((char*)(s).data) : NULL)
char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
//void decodeblock(unsigned char in[], char *clrstr);
//void b64_decode(char *b64src, char *clrdst);
void decodeblock(unsigned char in[], char *clrstr) {
    unsigned char out[4];
    out[0] = in[0] << 2 | in[1] >> 4;
    out[1] = in[1] << 4 | in[2] >> 2;
    out[2] = in[2] << 6 | in[3] >> 0;
    out[3] = '\0';
    strncat(clrstr, (char *)out, sizeof((char *)out));
}
void b64_decode(char *b64src, char *clrdst) {
    int c, phase, i;
    unsigned char in[4];
    char *p;

    clrdst[0] = '\0';
    phase = 0; i=0;
    while(b64src[i]) {
        c = (int) b64src[i];
        if(c == '=') {
            decodeblock(in, clrdst);
            break;
        }
        p = strchr(b64, c);
        if(p) {
            in[phase] = p - b64;
            phase = (phase + 1) % 4;
            if(phase == 0) {
                decodeblock(in, clrdst);
                in[0]=in[1]=in[2]=in[3]=0;
            }
        }
        i++;
    }
}

typedef struct {
    ngx_flag_t enabled;
    ngx_str_t forward_location;
    ngx_str_t cookie_name;
    ngx_str_t header_name;
} zapi_loc_conf_t;

static char *ngx_http_zapi_throttling(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

void finish_with_error(MYSQL *con);

struct ngx_http_mysql_srv_conf_s {
    ngx_str_t host;
    ngx_str_t user;
    ngx_str_t password;
    ngx_str_t database;
};

char *strdup(const char *original);
char *strdup(const char *original) {
    char *duplicate = malloc(strlen(original) + 1);
    if (duplicate == NULL) { return NULL; }

    strcpy(duplicate, original);
    return duplicate;
}
static ngx_command_t  ngx_http_zapi_throttling_commands[] = {

        { ngx_string("zapi_throttling"),
          NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
          ngx_http_zapi_throttling,
          0,
          0,
          NULL },
        ngx_null_command
};


static u_char  ngx_hello_world[] = "<link href=\"http://getbootstrap.com/dist/css/bootstrap.min.css\" rel=\"stylesheet\"><div class=\"starter-template\" "
        "       style=\"text-align:center; margin-top:250px;\">\n"
        "        <h1>Zapi Throttling Will be start from here...</h1>\n"
        "        <p class=\"lead\">Use this document as a way to quickly start any new project.<br> All you get is this text and a mostly barebones HTML document.</p>\n"
        "      </div>";

static ngx_http_module_t  ngx_http_zapi_throttling_module_ctx = {
        NULL,                          /* preconfiguration */
        NULL,                          /* postconfiguration */

        NULL,                          /* create main configuration */
        NULL,                          /* init main configuration */

        NULL,                          /* create server configuration */
        NULL,                          /* merge server configuration */

        NULL,                          /* create location configuration */
        NULL                           /* merge location configuration */
};


ngx_module_t ngx_http_zapi_throttling_module = {
        NGX_MODULE_V1,
        &ngx_http_zapi_throttling_module_ctx, /* module context */
        ngx_http_zapi_throttling_commands,   /* module directives */
        NGX_HTTP_MODULE,               /* module type */
        NULL,                          /* init master */
        NULL,                          /* init module */
        NULL,                          /* init process */
        NULL,                          /* init thread */
        NULL,                          /* exit thread */
        NULL,                          /* exit process */
        NULL,                          /* exit master */
        NGX_MODULE_V1_PADDING
};


static ngx_int_t ngx_http_zapi_throttling_handler(ngx_http_request_t *r)
{
    ngx_buf_t    *b;
    ngx_chain_t   out;
 //    ngx_http_mysql_srv_conf_t *conf = ngx_http_get_module_srv_conf(r, ngx_http_zapi_throttling_module);

    r->headers_out.content_type.len = sizeof("text/html") - 1;
    r->headers_out.content_type.data = (u_char *) "text/html";

    if (r->headers_in.authorization == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "Without Authorization headers \n"
                      "We don't need to do anything with this request");

    }else{

        ngx_str_t jwt = r->headers_in.authorization->value;

        char *myString = NGXCSTR(jwt);
        char dest[strlen(myString)];

        strncpy(dest, myString,4);
        dest[4] = 0; //null terminate destination
        if(strcmp("JWT ",dest) == 0){


            int len = strlen(myString);
            char fa[len];
            strncpy(fa, myString, len);
            char *t;

            t = strtok(fa, ".");
            char ts[sizeof(fa)];
            int i=0;
            while( t != NULL )
            {
                if(i==1){
                    strncpy(ts, t, sizeof(ts));
                }
                t = strtok(NULL, " ");
                i++;
            }

            int len2 = strlen(ts);
            char fb[len2];
            strncpy(fb, ts, len2);
            char *t2;

            t2 = strtok(fb, ".");
            char token_str_2[sizeof(fb)];
            int index2=0;
            while( t2 != NULL )
            {
                if(index2==0){
                    strncpy(token_str_2, t2, sizeof(token_str_2));
                }
                t2 = strtok(NULL, " ");
                index2++;
            }


            char md[strlen(token_str_2)];
            b64_decode(token_str_2, md);

            char *t3;
            char ls[strlen(md)];
            t3 = strtok(md, ",");

            while( t3 != NULL )
            {
                if(strstr(t3, "iss") != NULL) {
                    strcpy(ls,t3);
                }
                t3 = strtok(NULL, ",");
            }

            int l=strlen(ls);
            char a[l],b[l];
            strcpy(a,&ls[6]);
            strcpy(b,&a[1]);
            b[strlen(b)-1]=0;


            char m2[strlen(b)];
            b64_decode(b, m2);

            char *t4;
            char tenant_key[strlen(m2)];
            int index_t=0;
            t4 = strtok(m2, " ");

            while( t4 != NULL )
            {
                if(index_t==0) {
                    strcpy(tenant_key,t4);
                }
                t4 = strtok(NULL, " ");
                index_t++;
            }


            MYSQL *con = mysql_init(NULL);

            if (con == NULL) {
                fprintf(stderr, "mysql_init() failed\n");
            }

            if (mysql_real_connect(con, "localhost",
                                   "root",
                                   "root",
                                   "zapi", 0, NULL, 0) == NULL) {
                finish_with_error(con);
            }



            char stringa[1024];
            sprintf(stringa, "INSERT INTO access_log (tenantKey, accessIP, accessTime) VALUES ('%s', '%s', NOW());",
                    tenant_key, NGXCSTR(r->connection->addr_text));
            puts(stringa);
//        char * query =  "INSERT INTO access_log "
//                "(tenantKey, accessIP, accessTime) "
//                "VALUES "
//                "('%s', '%s', NOW());";
//        sprintf(query, "Number of fingers making up a hand are %f", fingers);
            ngx_str_t output;
            output.len = sizeof(stringa) - 1;
            output.data = (u_char *) stringa;
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, (char *) output.data);

            //insert into mysql database
            if (mysql_query(con, stringa)) {
                finish_with_error(con);
            }



        }
    }

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

    out.buf = b;
    out.next = NULL;

    b->pos = ngx_hello_world;
    b->last = ngx_hello_world + sizeof(ngx_hello_world);
    b->memory = 1;
    b->last_buf = 1;

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = sizeof(ngx_hello_world);
    ngx_http_send_header(r);

    return ngx_http_output_filter(r, &out);
}


static char *ngx_http_zapi_throttling(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_zapi_throttling_handler;

    return NGX_CONF_OK;
}

void finish_with_error(MYSQL *con)
{
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
}


