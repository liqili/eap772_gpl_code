# $Id: Makefile,v 1.40 2006/02/13 03:06:25 quozl Exp $
VERSION=0.9.0
RELEASE=

CC = mips-linux-gcc
RM	= rm -f
CFLAGS  = -Wall -Wno-trigraphs -O2 -fomit-frame-pointer

#TP_WEB_LOG=y

#ifdef TP_WEB_LOG
#CFLAGS += -DTP_WEB_LOG
#endif

#ifdef TP_WEB_LOG
#LDFLAGS += -lmsglog
#endif

#added by ZQQ, 10.10.09
ifdef TP_WEB_LOG
CFLAGS  += -I $(CONFIG_HTTPD_PATH)/msglogd/ -DTP_WEB_LOG
LDFLAGS += -L $(CONFIG_HTTPD_PATH)/msglogd/ -lmsglog
endif
#end 
TARGET = ipcserver
all: $(TARGET)

IPCSERVER_OBJS = ipcserver.o dhcpc.o http.o

ipcserver:ipcserver.o
$(IPCSERVER): $(IPCSERVER_OBJS) 
	$(CC) $(CFLAGS) $(LDFLAGS) -o ipcserver

dhcpc:dhcpc.o
$(IPCSERVER): $(IPCSERVER_OBJS) 
	$(CC) $(CFLAGS) $(LDFLAGS) -o dhcpc
	
dhcpc:http.o
	$(CC) http.c $(CFLAGS) $(LDFLAGS) -o http

clean:
	$(RM) *.o $(IPCSERVER) 

