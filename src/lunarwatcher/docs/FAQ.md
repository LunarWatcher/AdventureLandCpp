# Q: Why does the SSL protocol fail to verify certificates? (UNIX)

This is some times a system issue. Googling the problem often results in several solutions. Running in Docker, however, none of these worked. If all other solutions fail, or you just want a quick and fast fix, IXWebSocket supports specifying a CA through an environment variable, specifically `SSL_CERT_FILE`, which you can place your CA in. By default, you might be able to use this:

```
export SSL_CERT_FILE=/etc/ssl/certs/ca-certificates.crt
```
## WARNING:

If this is set already (check with `echo $SSL_CERT_FILE`), replacing it might have unintended consequences. 

This is specific for Debian, Ubuntu, and presumably their derivatives. For other variants, see [this](https://serverfault.com/a/722646). 

If even this fails, try using cURL's cert:
```
wget http://curl.haxx.se/ca/cacert.pem
```

... and store that somewhere, and point the SSL_CERT_FILE variable to it. 

