#!/bin/sh
echo "***remove old cscope and tags file***"
rm cscope* tags

files="./
       "
#./wireless/wlan_9.5.3.16/
#       ./wireless/qca-wifi-10.4.2.3/
#files="./wireless/qca-wifi-10.4.2.3/"

echo "***build files list***"
find ${files} -name "*.c" -o -name "*.h" >/tmp/cscope.files 

echo "***rebuild cscope and tags file***"
cscope -Rbq -i /tmp/cscope.files 
ctags -L /tmp/cscope.files 
