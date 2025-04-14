#!/bin/sh

RSA_KEY=/tmp/dropbear_rsa_host_key
DSS_KEY=/tmp/dropbear_dss_host_key
ECDSA_521_KEY=/tmp/dropbear_ecdsa_521_host_key
SSH_PORT=/tmp/sshPort

if ! test -f $RSA_KEY; then /usr/local/sbin/dropbearkey -t rsa -s 1024 -f $RSA_KEY; fi;
if ! test -f $DSS_KEY; then /usr/local/sbin/dropbearkey -t dss -f $DSS_KEY; fi;
if ! test -f $ECDSA_521_KEY; then /usr/local/sbin/dropbearkey -t ecdsa -s 521 -f $ECDSA_521_KEY; fi;

if ! test -f $SSH_PORT
then
	echo "no file $SSH_PORT"
else
	port=$(cat $SSH_PORT)
	if [ $SSH_PORT != 0 ]
	then
		/usr/local/sbin/dropbear -p $port -r $RSA_KEY -d $DSS_KEY -r $ECDSA_521_KEY
	fi
fi

