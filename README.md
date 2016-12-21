# Nginx-API-Throttling
nginx api throttling


## Config with zapi module

`./configure --add-module=/Users/masudurrahman/Desktop/zapi_throttling $(`mysql_config --cflags --libs`)`


## Install NGINX
`make && make install`


## Run NGINX

- `path/to/nginx` working directory

- `sudo sbin/nginx` start

- `sudo sbin/nginx -s quit` quit

## Logs
- `tail -f path/to/nginx/logs/access.log` access log
- `tail -f path/to/nginx/logs/error.log` error log


## MySQL setup
- create db `zapi`
- create table `access_log` -field (tenantKey, accessIP, accessTime)
   
 
