# opkg over tar over curl

## Test
1. before testing, you need start a http server that provide Tar file.
``` bash
cd test/data
python3 -m http.server --bind 127.0.0.1 7800
```
