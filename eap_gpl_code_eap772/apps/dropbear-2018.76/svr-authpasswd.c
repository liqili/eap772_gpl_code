/*
 * Dropbear - a SSH2 server
 * 
 * Copyright (c) 2002,2003 Matt Johnston
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

/* Validates a user password */

#include "includes.h"
#include "session.h"
#include "buffer.h"
#include "dbutil.h"
#include "auth.h"
#include "runopts.h"
#include "tp_md5.h"
#include "tp_auth.h"

#define DROPBEAR_PWD_FILE                "/tmp/dropbear_info"
#define DROPBEAR_USERNAME_LEN   64
#define DROPBEAR_PASSWORD_LEN   64
#define TMP_BUF_LEN					512

#if DROPBEAR_SVR_PASSWORD_AUTH

/* not constant time when strings are differing lengths. 
 string content isn't leaked, and crypt hashes are predictable length. */
static int constant_time_strcmp(const char* a, const char* b) {
	size_t la = strlen(a);
	size_t lb = strlen(b);

	if (la != lb) {
		return 1;
	}

	return constant_time_memcmp(a, b, la);
}

/* Process a password auth request, sending success or failure messages as
 * appropriate */
/* modified by chenjianfeng */
void svr_auth_password(char *userName) {
	char * passwdcrypt = NULL; /* the crypt from /etc/passwd or /etc/shadow */
	char * testcrypt = NULL; /* crypt generated from the user's password sent */
	char * password;
	unsigned int passwordlen;
	unsigned int changepw;
	
if (strcmp(userName, "root") != 0){
	char fileUserName[DROPBEAR_USERNAME_LEN + 1] = {0};
	char filePwdCrypt[DROPBEAR_PASSWORD_LEN + 1]= {0};
	char nameCrypt [DROPBEAR_USERNAME_LEN] = {0};
	char pwdCrypt [DROPBEAR_PASSWORD_LEN] = {0};
	char buf[TMP_BUF_LEN] = {0};
	int length = 0;
	FILE* pwdFD = NULL;
	char *pTemp = NULL;
	
	pwdFD = fopen(DROPBEAR_PWD_FILE, "r+");
	if(NULL == pwdFD)
	{
		printf("Open the write file error!\n");
		goto error_leave;
	}

	/* check if client wants to change password */
	changepw = buf_getbool(ses.payload);
	if (changepw) {
		/* not implemented by this server */
		fclose(pwdFD);
		goto error_leave;
	}
	
	password = buf_getstring(ses.payload, &passwordlen);
	cal_md5((unsigned char*)pwdCrypt, (unsigned char*)password, passwordlen);

	while(fgets(buf, TMP_BUF_LEN, pwdFD))
	{
		if (NULL != (pTemp = strchr(buf, '\n')))
		{
			*pTemp = '\0';
		}
		
		length = strlen(buf);

		if((strstr(buf, "username:"))
			&& ((length - strlen("username:")) <= DROPBEAR_USERNAME_LEN))
		{
			strncpy(fileUserName, buf + strlen("username:"), DROPBEAR_USERNAME_LEN);
		}
		else if((strstr(buf, "password:"))
			&& ((length - strlen("password:")) <= DROPBEAR_PASSWORD_LEN))
		{
			strncpy(filePwdCrypt, buf + strlen("password:"), DROPBEAR_PASSWORD_LEN);
		}
		else
		{
			printf("Read the file error!\n");
			fclose(pwdFD);
			goto error_leave;
		}
		memset(buf, 0, sizeof(buf));
	}
	fclose(pwdFD);
	
	if(strcmp(userName, fileUserName))
	{
		goto error_leave;
	}
	else if(strcmp(pwdCrypt, filePwdCrypt))
	{
		goto error_leave;
	}
	else
	{
	}

	tpUpdateAuthRecord(AUTH_SUCCESS);
	send_msg_userauth_success();
	return;
	
error_leave:
	tpUpdateAuthRecord(AUTH_FAILED);
	send_msg_userauth_failure(0, 1);
	return;

} else {

	passwdcrypt = ses.authstate.pw_passwd;

#ifdef DEBUG_HACKCRYPT
	/* debugging crypt for non-root testing with shadows */
	passwdcrypt = DEBUG_HACKCRYPT;
#endif

	/* check if client wants to change password */
	changepw = buf_getbool(ses.payload);
	if (changepw) {
		/* not implemented by this server */
		send_msg_userauth_failure(0, 1);
		return;
	}

	password = buf_getstring(ses.payload, &passwordlen);

	/* the first bytes of passwdcrypt are the salt */
	testcrypt = crypt(password, passwdcrypt);
	m_burn(password, passwordlen);
	m_free(password);

	if (testcrypt == NULL) {
		/* crypt() with an invalid salt like "!!" */
		dropbear_log(LOG_WARNING, "User account '%s' is locked",
				ses.authstate.pw_name);
		send_msg_userauth_failure(0, 1);
		return;
	}

	/* check for empty password */
	if (passwdcrypt[0] == '\0') {
		dropbear_log(LOG_WARNING, "User '%s' has blank password, rejected",
				ses.authstate.pw_name);
		send_msg_userauth_failure(0, 1);
		return;
	}

	if (constant_time_strcmp(testcrypt, passwdcrypt) == 0) {
		/* successful authentication */
		dropbear_log(LOG_NOTICE, 
				"Password auth succeeded for '%s' from %s",
				ses.authstate.pw_name,
				svr_ses.addrstring);
		send_msg_userauth_success();
	} else {
		dropbear_log(LOG_WARNING,
				"Bad password attempt for '%s' from %s",
				ses.authstate.pw_name,
				svr_ses.addrstring);
		send_msg_userauth_failure(0, 1);
	}
}
}

#endif
