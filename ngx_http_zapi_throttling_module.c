//
// Created by Masudur Rahman on 12/17/16.
//



#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <string.h>
#include <time.h>
#include <mysql.h>


#define ZAPI_THROTTLING_MODULE_VERSION "0.1"

typedef struct {
    ngx_flag_t enabled;
    ngx_str_t forward_location;
    ngx_str_t cookie_name;
    ngx_str_t header_name;
} zapi_loc_conf_t;

static char *ngx_http_zapi_throttling(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

void finish_with_error(MYSQL *con);

struct ngx_http_mysql_srv_conf_s {
    char * host;
    char * user;
    char * password;
    char * database;

};

typedef struct ngx_http_mysql_srv_conf_s ngx_http_mysql_srv_conf_t;

static ngx_command_t  ngx_http_zapi_throttling_commands[] = {

        { ngx_string("zapi_throttling"),
          NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
          ngx_http_zapi_throttling,
          0,
          0,
          NULL },
        {	ngx_string("mysql_host"),
             NGX_HTTP_SRV_CONF|NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
             ngx_conf_set_str_slot,
             NGX_HTTP_SRV_CONF_OFFSET,
             offsetof(ngx_http_mysql_srv_conf_t, host),
             NULL },

        {	ngx_string("mysql_user"),
             NGX_HTTP_SRV_CONF|NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
             ngx_conf_set_str_slot,
             NGX_HTTP_SRV_CONF_OFFSET,
             offsetof(ngx_http_mysql_srv_conf_t, user),
             NULL },

        {	ngx_string("mysql_password"),
             NGX_HTTP_SRV_CONF|NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
             ngx_conf_set_str_slot,
             NGX_HTTP_SRV_CONF_OFFSET,
             offsetof(ngx_http_mysql_srv_conf_t, password),
             NULL },

        {	ngx_string("mysql_database"),
             NGX_HTTP_SRV_CONF|NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
             ngx_conf_set_str_slot,
             NGX_HTTP_SRV_CONF_OFFSET,
             offsetof(ngx_http_mysql_srv_conf_t, database),
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
    ngx_str_t dec_jwt;
    ngx_http_mysql_srv_conf_t *conf = ngx_http_get_module_srv_conf(r, ngx_http_zapi_throttling_module);

    r->headers_out.content_type.len = sizeof("text/html") - 1;
    r->headers_out.content_type.data = (u_char *) "text/html";

    if (r->headers_in.authorization == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "Without Authorization headers \n"
                      "We don't need to do anything with this request");

    }else{

        ngx_str_t jwt = r->headers_in.authorization->value;
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Authorization: \"%V\"", &jwt);
        dec_jwt.data = (u_char*)ngx_pcalloc(r->pool, jwt.len + 1);
        ngx_decode_base64(&dec_jwt, &jwt);
        dec_jwt.data[dec_jwt.len] = 0;
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "DEC string is %V\n", &dec_jwt);
        MYSQL *con = mysql_init(NULL);

        if (con == NULL)
        {
            fprintf(stderr, "mysql_init() failed\n");
        }

        if (mysql_real_connect(con, conf->host,
                               conf->user,
                               conf->password,
                               conf->database, 0, NULL, 0) == NULL)
        {
            finish_with_error(con);
        }

        char * query =  "INSERT INTO access_log "
                "(tenantKey, accessIP, accessTime) "
                "VALUES "
                "('tenantKey', '127.0.0.1', NOW());";

        //insert into mysql database
        if (mysql_query(con, query))
        {
            finish_with_error(con);
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
