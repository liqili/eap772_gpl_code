/*! Copyright(c) 2014 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     make_flash.c
 *\brief    Tool for generating firmware
 *
 *\author   Wu Kan
 *\version  1.0.0
 *\date     25/Jan/2019
 *
 *\history \arg 
 *\01       25/Jan/2019     Wu Kan     Create the file.
 */
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdint.h>

#include "make_flash.h"
#include "md5_interface.h"
#include "cJSON.h"

static unsigned char md5Key[16] = 
{
    0x7a, 0x2b, 0x15, 0xed,  0x9b, 0x98, 0x59, 0x6d,
    0xe5, 0x04, 0xab, 0x44,  0xac, 0x2a, 0x9f, 0x4e
};

unsigned char tpHeaderMagicNumber[MAGIC_LEN] = 
{
    0xAA, 0x55, 0x9D, 0xD1, 0xA8, 0xC8, 0x83, 0x31, 0xC9, 0x69,
    0xFB, 0xBF, 0xBC, 0xF0, 0xD4, 0x32, 0x70, 0xC7, 0x55, 0xAA
 };

unsigned char upgradeHeaderMagicNumber[MAGIC_LEN] = 
{
    0xAA, 0x55, 0x4C, 0x5E, 0x83, 0x1F, 0x53, 0x4B, 0xA1, 0xF8,
    0xF7, 0xC9, 0x18, 0xDF, 0x8F, 0xBF, 0x7D, 0xA1, 0x55, 0xAA
};


NM_PTN_STRUCT *pNmPtnStruct;
NM_PTN_STRUCT nmPtnStruct;
char g_imgGen[NM_LONG_STRLEN] = {0};

NM_FLASHER_PTN_STRUCT *pFlasherPtnTbl;
NM_FLASHER_PTN_STRUCT flasherPtnTbl;

SOFT_VER_STRUCT softverInfo;
char fileNameStr[NM_LONG_STRLEN] = {0};

char productInfoPath[64];

unsigned int blockSizeKiB = 0;
unsigned long blockSize = 0;

int debugOn = FALSE;
int manuOn  = FALSE;
int genOOB = FALSE;

#define SIZE_KIB(x)		(x * 1024)

#define DEBUG_PRINTF(fmt, args...) \
    do{\
        if(debugOn)\
            printf(fmt, ##args);\
    }while(0)

long get_file_size(const char *path)
{
    long filesize = -1;
    struct stat statbuff;
    if (0 == strcmp(path, NM_FILE_EMPTY))
    {
        return 0;
    }
    if (stat(path, &statbuff) < 0)
    {
        printf("Path: \"%s\" does not exist.\n", path);
        return filesize;
    }
    else
    {
        filesize = statbuff.st_size;
    }
    return filesize;
}

/* hex to str, note : strBuf must be cleared to 0!!!! */
int hex2str(unsigned char *hexBuf, int hexLen, unsigned char* strBuf)
{
    int ret = -1;
    int i = 0;
    unsigned char temp_hex_str[4];

    if (hexBuf == NULL || strBuf == NULL)
    {
        goto out;
    }

    for (i = 0; i < hexLen; i++)
    {
        memset(temp_hex_str, 0, 4);
        sprintf(temp_hex_str, "%02X", hexBuf[i]);
        strcat(strBuf, temp_hex_str);
    }

    ret = 0;
out:
    return ret;
}

int str2hex(unsigned char *hexBuf, unsigned char* strBuf)
{
    int ret = -1;
    int i = 0;
    unsigned char* strPtr;
    unsigned char strArr[3] = {'\0'};
    
    if (hexBuf == NULL || strBuf == NULL)
    {
        goto out;
    }

    strPtr = strBuf;
    while (*strPtr != '\0')
    {
        strArr[0] = *strPtr;
        strArr[1] = *(++strPtr);
        hexBuf[i] = (unsigned char)strtoul(strArr, NULL, 16);
        i++;
        strPtr++;
    }

    ret = 0;
out:
    return ret;
}

int strUpper(unsigned char strBuf[])
{
    int strLen = strlen(strBuf);
    int index = 0;

    for(index = 0; index < strLen; index++)
    {
        if(strBuf[index] >= 'a' && strBuf[index] <= 'z')
        {
            strBuf[index] -= 32;
        }
    }
    return 0;
}

unsigned int hostToLittleEndian32(unsigned int value)
{
    int tempValue = htonl(value);
    char *pData = &tempValue;

    pData[0] = pData[0] ^ pData[3];
    pData[3] = pData[0] ^ pData[3];
    pData[0] = pData[0] ^ pData[3];

    pData[1] = pData[1] ^ pData[2];
    pData[2] = pData[1] ^ pData[2];
    pData[1] = pData[1] ^ pData[2];
    
    return tempValue;
}

int insert_ecc_to_image(char *in_file_name, char *out_file_name)
{
    char cmd[512] = {0};
    snprintf(cmd, 512, "%s %s -h -4 -o %s >>/dev/null", g_imgGen, in_file_name, out_file_name);
    system(cmd);
    return 0;
}

int insert_oob128_ecc8_to_image(char *in_file_name, char *out_file_name)
{
    char cmd[512] = {0};
    snprintf(cmd, 512, "./img_gen_oob128 %s -h -8 -O 128 -o %s > /dev/null", in_file_name, out_file_name);
    system(cmd);
    return 0;
}

static void strIgnrSpace(char *pStr, int size)
{
    int index = 0, num = 0;
    char tmp[NM_LONG_STRLEN] = {0};
    char *pCur;

    if (NULL == pStr)
    {
        return;
    }

    for (num = 0, pCur = pStr; num < size && '\0' != *pCur; num ++, pCur ++)
    {
        if ((' ' != *pCur) && ('\t' != *pCur) && ('\r' != *pCur) && ('\n' != *pCur))
        {
            tmp[index++] = *pCur;
        }
    }
    tmp[index++] = '\0';

    memcpy(pStr, tmp, index);

    return;
}

int parsePtnTblFile(NM_PTN_STRUCT *pNmPtn)
{
    int ret = 0;
    int index = 0, noused = 0;
    int num_read = 0;
    size_t len = 0;
    char* line = 0;	
    FILE *ptn_file = 0;
    int assigns;
    struct timespec curTime;
    struct tm* tm;
    long actualsize = 0;
    

    if (NULL == pNmPtn || 0 == pNmPtn->ptnTblFile[0])
    {
        printf("-p, --partition=FILE      Partition file\n");
        return -1;
    }

    ptn_file = fopen(pNmPtn->ptnTblFile, "r");
    
    if (!ptn_file)
    {
        printf("failed to open partition file.\n");
        return -1;
    }

    if ((num_read = getline(&line, &len, ptn_file)) != -1)
    {
        strIgnrSpace(line, num_read);
        assigns = sscanf(line, "total=%d,flash=%dM", &pNmPtn->ptnNum, &pNmPtn->flashSize);

        switch(pNmPtn->flashSize)
        {
        case FLASH_EM_2M:
            pNmPtn->flashSize = BUF_LEN_2M;
            break;
        case FLASH_EM_4M:
            pNmPtn->flashSize = BUF_LEN_4M;
            break;
        case FLASH_EM_8M:
            pNmPtn->flashSize = BUF_LEN_8M;
            break;
        case FLASH_EM_16M:
            pNmPtn->flashSize = BUF_LEN_16M;
            break;
        case FLASH_EM_32M:
            pNmPtn->flashSize = BUF_LEN_32M;
            break;
        case FLASH_EM_128M:
            pNmPtn->flashSize = BUF_LEN_128M;
            break;
        default:
            printf("flash=2M/4M/8M/16M/32M/128M\n");
            pNmPtn->flashSize = 0;
            break;
        }
        
        if (0 == pNmPtn->ptnNum || NM_PTN_NUM_MAX < pNmPtn->ptnNum)
        {
            printf("total=%d out of range\n", pNmPtn->ptnNum);

            ret = -1;
            goto LEAVE;
        }
        free(line);
        line = NULL;
    }
        
    for (index = 0; index < pNmPtn->ptnNum; index ++)
    {
        if ((num_read = getline(&line, &len, ptn_file)) == -1)
            break;
        
        strIgnrSpace(line, num_read);
        
        assigns = sscanf(line, "%d=%[^,],%x,%x,%d,%d,%d,%d,%s", 
            &noused, pNmPtn->entries[index].name, 
            &pNmPtn->entries[index].base, &pNmPtn->entries[index].size, 
            &pNmPtn->entries[index].contentType, &pNmPtn->entries[index].writeCap, 
            &pNmPtn->entries[index].rootType, &pNmPtn->entries[index].writeType, 
            pNmPtn->entries[index].filePath);

        if (9 != assigns || noused != index)
        {
            printf("partition table: index=%d error\n", index);

            ret = -1;
            goto LEAVE;
        }
        if (pNmPtn->entries[index].base % blockSize)
        {
            printf("partition table: index=%d base error\n", index);
            ret = -1;
            goto LEAVE;
        }
        if (pNmPtn->entries[index].size % blockSize)
        {
            printf("partition table: index=%d size error\n", index);
            ret = -1;
            goto LEAVE;
        }
        pNmPtn->entries[index].baseBlock = pNmPtn->entries[index].base / blockSize;
        pNmPtn->entries[index].blocks = pNmPtn->entries[index].size / blockSize;
        if (pNmPtn->entries[index].writeType != 0)
        {
            if ((pNmPtn->entries[index].actualSize = get_file_size(pNmPtn->entries[index].filePath)) < 0)
            {
                printf("Error in get size for %s.\n", pNmPtn->entries[index].filePath);
                ret = -1;
                goto LEAVE;
            }
            pNmPtn->entries[index].actualBlocks = (pNmPtn->entries[index].actualSize % blockSize ?
                pNmPtn->entries[index].actualSize / blockSize + 1 : pNmPtn->entries[index].actualSize / blockSize);
            pNmPtn->entries[index].actualSizePadded = pNmPtn->entries[index].actualBlocks * blockSize;
        }
        free(line);
        line = NULL;
    }

LEAVE:
    if (line != NULL)  
        free(line);
        
    fclose(ptn_file);

    return ret;
}

static void debug_printPtnTbl(NM_PTN_STRUCT *pNmPtn)
{
    int index = 0;
    
    DEBUG_PRINTF("================ partition table =========================\n");
    DEBUG_PRINTF("total=%d, flash=0x%08x\n", pNmPtn->ptnNum, pNmPtn->flashSize);

    for (index = 0; index < pNmPtn->ptnNum; index ++)
    {
        DEBUG_PRINTF("%d= %16s, 0x%08x, 0x%08x, %d, %d, %d, %d, baseBlock: %d, %s\n",
            index, pNmPtn->entries[index].name, pNmPtn->entries[index].base,
            pNmPtn->entries[index].size, pNmPtn->entries[index].contentType,
            pNmPtn->entries[index].writeCap, pNmPtn->entries[index].rootType,
            pNmPtn->entries[index].writeType, pNmPtn->entries[index].baseBlock,
            pNmPtn->entries[index].filePath);
    }

    DEBUG_PRINTF("================ partition table end ======================\n");
    
    return;
}

int chkPtnTblData(NM_PTN_STRUCT *pNmPtn)
{
    int index;
    FILE *FP;
    int actualSize;
    uint32_t lastBase = 0;
    uint32_t lastSize = 0;
    for (index = 0; index < pNmPtn->ptnNum; index ++)
    {
        if (pNmPtn->entries[index].contentType == NM_PTN_CONTENT_MTD)
        {
            if (pNmPtn->entries[index].size < pNmPtn->entries[index].actualSize)
            {
                printf("partition %d does not have enough space\n", index);
                return -1;
            }
            if (pNmPtn->entries[index].base < (lastBase + lastSize))
            {
                printf("patition table: index=%d error.\n", index);
                return -1;
            }
            else
            {
                lastBase = pNmPtn->entries[index].base;
                lastSize = pNmPtn->entries[index].size;
            }

            if (NULL == pNmPtn->entries[index].filePath)
            {
                printf("patition table(index=%d): file is null.\n", index);
                return -1;
            }

            if (0 != strcmp(pNmPtn->entries[index].filePath,NM_FILE_EMPTY))
            {
                if (pNmPtn->entries[index].actualSize > lastSize)
                {
                    printf("file(%s) is too large, actualSize = %x, lastSize = %x.\n",pNmPtn->entries[index].filePath,
							pNmPtn->entries[index].actualSize, lastSize);
                    return -1;
                }
            }
        }
    }

    if ((lastBase + lastSize) > pNmPtn->flashSize)
    {
        printf("patition table: index=%d error.\n", index);
        return -1;
    }
    
    return 0;
};

static int makeFileNameStr()
{
	int index = 0;
	NM_PTN_ENTRY *pEntry = NULL;
	FILE *ptnFile = NULL;
	
	/* search soft-version file in ptn table */
	for (index = 0; index < pNmPtnStruct->ptnNum; index++)
	{
		if (0 == strcmp(pNmPtnStruct->entries[index].name, PTN_SOFT_VERSION_NAME))
		{
			pEntry = &(pNmPtnStruct->entries[index]);
			printf("found %s entry\n", PTN_SOFT_VERSION_NAME);
			break;
		}
	}

	if (pEntry)
	{
		ptnFile = fopen(pEntry->filePath, "r");
    
	    if (!ptnFile)
	    {
	        printf("failed to open file.\n");
	        return -1;
	    }

		if (1 != fread(&softverInfo, sizeof(SOFT_VER_STRUCT), 1, ptnFile))
		{
			printf("read file content failed\n");
			fclose(ptnFile);
			return -1;
		}

		softverInfo.sn = ntohl(softverInfo.sn);
		softverInfo.buildDate = ntohl(softverInfo.buildDate);
		softverInfo.releaseTime = ntohl(softverInfo.releaseTime);
		
		snprintf(fileNameStr, NM_LONG_STRLEN, "%d.%d.%d_[%8x-rel%d]",
            		((softverInfo.sn >> 16) & 0xff), 
            		((softverInfo.sn >> 8) & 0xff), 
            		(softverInfo.sn & 0xff), 
            		softverInfo.buildDate, 
            		softverInfo.releaseTime);

		fclose(ptnFile);
		return 0;
	}
	else
	{
		printf("no %s PTN\n", PTN_SOFT_VERSION_NAME);
		return -1;
	}
}


static int getFileContentToBuf(char* buf, unsigned int BUFLEN, unsigned int *actualSize, char* fileName)
{
    int ret = 0;
    FILE *FP;
    *actualSize = 0;
        
    if (fileName == NULL)
    {
        ret = -1;
        return ret;
    }	
    
    if((FP = fopen(fileName,"rb")) == NULL)
    {
        printf("open file(%s) error.\n",fileName);
        return -1;
    }
    fseek(FP,0,SEEK_END);
    *actualSize = ftell(FP);

    if (*actualSize >= BUFLEN)
    {
        printf("file(%s) is too large.\n",fileName);
        ret = -1;
        goto LEAVE;
    } 
    
    fseek(FP,0,SEEK_SET);
    fread(buf,1,*actualSize,FP);

LEAVE:
    fclose(FP);	
    return ret;
}

/* Data in NM_PTN_CONTENT_USER can be updated in manu firmware.
 * Data in NM_PTN_CONTENT_MANU can only be changed during manufacturing.
 */
static int isUpContent(NM_PTN_ENTRY *pEntry, int fileType)
{
    int ret = FALSE;
    switch (fileType) {
    case FILE_UP:
        if (FW_TYPE_ENABLED(pEntry->writeType, FW_TYPE_UP))
        {
            ret = TRUE;
        }
        break;
    case FILE_MANU:
        if (FW_TYPE_ENABLED(pEntry->writeType, FW_TYPE_MANU))
        {
            ret = TRUE;
        }
        break;
    default:
        break;
    }
    return ret;
}

static int isFlashContent(NM_PTN_ENTRY *pEntry)
{
    int ret = FALSE;
    if (pEntry->contentType == NM_PTN_CONTENT_MTD)
    {
        ret = TRUE;
    }
    return ret;
}

static int makeUpContent(int index, char *buf, int bufLen, int *fileSize)
{
    int ret = -1;
    
    NM_PTN_ENTRY *pEntry = &nmPtnStruct.entries[index];
    if (getFileContentToBuf(buf, bufLen, fileSize, pEntry->filePath) == -1)
    {
        printf("fail to get file content. filePath = %s\n", pEntry->filePath);
        goto LEAVE;
    }
    
    if (*fileSize != pEntry->actualSize)
    {
        printf("file size changed. index=%d\n", index);
        goto LEAVE;
    }
    ret = 0;

LEAVE:
    return ret;
}

static int makeFlashContent(int index, char *buf, int bufLen, int *fileSize)
{
    int ret = -1;

    NM_PTN_ENTRY *pEntry = &nmPtnStruct.entries[index];
    if (getFileContentToBuf(buf, bufLen, fileSize, pEntry->filePath) == -1)
    {
        printf("fail to get file content. filePath = %s\n", pEntry->filePath);
        goto LEAVE;
    }

    if (*fileSize != pEntry->actualSize)
    {
        printf("file size changed. index = %d\n", index);
        goto LEAVE;
    }

    *fileSize = pEntry->size;
    ret = 0;

LEAVE:
    return ret;
}

static int isMainRoot(NM_PTN_ENTRY *pEntry)
{
    int ret = FALSE;
    if (pEntry->contentType == NM_PTN_CONTENT_MTD &&
        pEntry->rootType == NM_ROOT_UP)
    {
        ret = TRUE;
    }
    return ret;
}

int makeUpFile(int fileType)
{
    int ret = -1;
    char *basicAddr;
    char *tmpFileBuf;
    char *fileAddr;
    unsigned char md5_digest[FWUP_HDR_MD5_LEN] = {'\0'};
    FWUP_FILE_HEAD fileHead = {0};

    int upFileSize = 0;
    int index;
    int upIndex = 0;
    int upBase = 0;

    NM_PTN_ENTRY *pEntry = NULL;
    NM_UP_PTN_ENTRY *pUpEntry = NULL;

    int out_fd = 0;
    char up_filename[NM_LONG_STRLEN] = {0};

    NM_UP_PTN_TBL_STRUCT upPtnTbl;
    NM_UP_PTN_TBL_STRUCT *pUpPtnTbl = &upPtnTbl;

    int bufLen = nmPtnStruct.flashSize + sizeof(FWUP_FILE_HEAD) + NM_PTN_TABLE_SIZE;
    int bufLeft = nmPtnStruct.flashSize;
    int ptnSize = 0;

    memset(pUpPtnTbl, 0, sizeof(NM_UP_PTN_TBL_STRUCT));

    basicAddr = (char *)malloc(bufLen);
    if (NULL == basicAddr)
    {
        printf("malloc error.\n");
        return ret;
    }
    memset(basicAddr, 0xFF, bufLen);

    tmpFileBuf = basicAddr + sizeof(FWUP_FILE_HEAD);

    fileAddr = tmpFileBuf + NM_PTN_TABLE_SIZE;
    upFileSize = NM_PTN_TABLE_SIZE;

    DEBUG_PRINTF("make upcontent start\n");
    /* Make root content for up firmware */
    for (index = 0; index < nmPtnStruct.ptnNum; index++)
    {
        pEntry = &nmPtnStruct.entries[index];
        if (isMainRoot(pEntry))
        {
            break;
        }
    }
    if (index >= nmPtnStruct.ptnNum)
    {
        printf("No root partition found.\n");
        goto LEAVE;
    }
    if (makeUpContent(index, fileAddr, bufLeft, &ptnSize) == -1)
    {
        printf("fail to make root content.\n");
        goto LEAVE;
    }
    upPtnTbl.rootSize = htonl(ptnSize);
    bufLeft -= ptnSize;
    fileAddr += ptnSize;
    upBase += ptnSize;
    upFileSize += ptnSize;
    
    /* make contents used during upgrading */
    for (index = 0; index < nmPtnStruct.ptnNum; index++)
    {
        pEntry = &nmPtnStruct.entries[index];
        if (isUpContent(pEntry, fileType) == TRUE)
        {
            if (makeUpContent(index, fileAddr, bufLeft, &ptnSize) == -1)
            {
                printf("fail make upcontent. index = %d\n", index);
                goto LEAVE;
            }
            bufLeft -= ptnSize;
            fileAddr += ptnSize;
            pUpEntry = &upPtnTbl.entries[upIndex];
            memcpy(pUpEntry->name, pEntry->name, NM_PTN_NAME_LEN);
            pUpEntry->base = htonl(upBase);
            pUpEntry->size = htonl(ptnSize);
            pUpEntry->content_type = htonl(pEntry->contentType);
            upBase += ptnSize;
            upFileSize += ptnSize;
            upPtnTbl.ptnNum = ++upIndex;
        }
    }
    upPtnTbl.ptnNum = htonl(upPtnTbl.ptnNum);
    upPtnTbl.isManu = htonl(fileType == FILE_MANU);

    DEBUG_PRINTF("make upcontent done\n");

    /* Add up header to up firmware */
    memcpy(tmpFileBuf, pUpPtnTbl, sizeof(NM_UP_PTN_TBL_STRUCT));

    upFileSize += sizeof(FWUP_FILE_HEAD);
    DEBUG_PRINTF("up file size: %d\n", upFileSize);

    fileHead.fileSize = htonl(upFileSize);
    memset(fileHead.modelId, 0xFF, FWUP_HDR_PRODUCT_ID_LEN);
	
    memcpy(basicAddr, &fileHead, sizeof(FWUP_FILE_HEAD));

    /* MD5 Header */
    memcpy(basicAddr+4, md5Key, FWUP_HDR_MD5_LEN);
    md5_make_digest(md5_digest, (unsigned char *)(basicAddr + 4), (upFileSize - 4));
    memcpy(fileHead.fileMd5, md5_digest, FWUP_HDR_MD5_LEN);
    memcpy(basicAddr, &fileHead, sizeof(FWUP_FILE_HEAD));

    if (fileType == FILE_UP)
    {
        snprintf(up_filename, NM_LONG_STRLEN, "%s_%s_up.bin", pNmPtnStruct->prefix, fileNameStr);
    }
    else if(fileType == FILE_MANU)
    {
        snprintf(up_filename, NM_LONG_STRLEN, "%s_%s_manu.bin", pNmPtnStruct->prefix, fileNameStr);
    }
    else
    {
        DEBUG_PRINTF("fileType error: %d\n", fileType);
        goto LEAVE;
    }

    DEBUG_PRINTF("up file name: %s\n", up_filename);

    out_fd = open(up_filename, O_RDWR | O_CREAT | O_TRUNC, 00777);
    if (out_fd < 0)
    {
        printf("fail to open file.\n");
        goto LEAVE;
    }

    write(out_fd, basicAddr, upFileSize);
    close(out_fd);
    ret = 0;

LEAVE:
    free(basicAddr);
    return ret;
}

static void initFlasherPtnTable(void)
{
    memset(pFlasherPtnTbl, 0, sizeof(NM_FLASHER_PTN_STRUCT));
    memset(pFlasherPtnTbl->entries, 0xFF, sizeof(NM_FLASHER_PTN_ENTRY) * NM_FLASHER_PTN_NUM_MAX);
}

int makeFlashFile(void)
{
    int ret = -1;
    int out_fd;
    int ptn_tbl_fd;
    char *tmpFileBuf;
    char *fileAddr;
    int ptnSize;
    int flashFileSize = 0;
    int bufLeft = 0;
    int index;
    int flashIndex = 0;
    char flash_filename[NM_LONG_STRLEN] = {0};
    char flash_filename_no_ecc[NM_LONG_STRLEN] = {0};
    char flash_filename_2[NM_LONG_STRLEN] = {0};
    char flash_ptn_tbl_filename[NM_LONG_STRLEN] = {0};
    NM_PTN_ENTRY *pEntry = NULL;
    NM_FLASHER_PTN_ENTRY *pFlashEntry = NULL;

    tmpFileBuf = malloc(nmPtnStruct.flashSize);
    fileAddr = tmpFileBuf;
    bufLeft = nmPtnStruct.flashSize;
    if (tmpFileBuf == NULL)
    {
        printf("malloc error.\n");
        return -1;
    }
    bufLeft = nmPtnStruct.flashSize;
    memset(tmpFileBuf, 0xFF, bufLeft);
    initFlasherPtnTable();

    DEBUG_PRINTF("make flash content start.\n");
    for (index = 0; index < pNmPtnStruct->ptnNum; index ++)
    {
        pEntry = &nmPtnStruct.entries[index];
        if (isFlashContent(pEntry) == TRUE)
        {
            if (strncmp(pEntry->filePath, NM_FILE_EMPTY, NM_LONG_STRLEN) == 0)
            {
                printf("Empty mtd part %s\n", pEntry->name);
                ptnSize = pEntry->size;
                fileAddr += ptnSize;
                bufLeft -= ptnSize;
                flashFileSize += ptnSize;
                printf("ptnSize: 0x%x, flashFileSize: 0x%x\n", ptnSize, flashFileSize);

                continue;
            }
            if (makeFlashContent(index, fileAddr, bufLeft, &ptnSize) == -1)
            {
                printf("fail to make flash content\n");
                goto LEAVE;
            }
            pFlashEntry = &flasherPtnTbl.entries[flashIndex];
            fileAddr += ptnSize;
            bufLeft -= ptnSize;
            flashFileSize += ptnSize;
            pFlashEntry->baseBlock = hostToLittleEndian32(pEntry->baseBlock);
            pFlashEntry->endBlock = hostToLittleEndian32(pEntry->baseBlock + pEntry->blocks - 1);
            pFlashEntry->actualBlocks = hostToLittleEndian32(pEntry->actualBlocks);
            DEBUG_PRINTF("flash ptn index = %d, baseBlock: 0x%x, endBlock: 0x%x, actualBlocks: 0x%x\n", flashIndex, pFlashEntry->baseBlock,
                pFlashEntry->endBlock, pFlashEntry->actualBlocks);
            flasherPtnTbl.ptnNum = ++flashIndex;
        }
    }
    DEBUG_PRINTF("make flash content done.\n");

    snprintf(flash_filename_no_ecc, NM_LONG_STRLEN, "%s_%s_flash_no_ecc.bin", nmPtnStruct.prefix, fileNameStr);
    snprintf(flash_filename, NM_LONG_STRLEN, "%s_%s_flash.bin", nmPtnStruct.prefix, fileNameStr);
    snprintf(flash_ptn_tbl_filename, NM_LONG_STRLEN, "%s_%s_flash.bin.partable", nmPtnStruct.prefix, fileNameStr);
    DEBUG_PRINTF("flash file name: %s\n", flash_filename);

    if (TRUE == genOOB)
    {
        snprintf(flash_filename_2, NM_LONG_STRLEN, "%s_%s_flash.OOB128.bin", nmPtnStruct.prefix, fileNameStr);
    }
    

    out_fd = open(flash_filename_no_ecc, O_RDWR | O_CREAT | O_TRUNC, 00777);
    if (out_fd < 0)
    {
        printf("failed to open file.\n");
        goto LEAVE;
    }

    write(out_fd, tmpFileBuf, flashFileSize);
    close(out_fd);

    ptn_tbl_fd = open(flash_ptn_tbl_filename, O_RDWR | O_CREAT | O_TRUNC, 00777);
    if (ptn_tbl_fd < 0)
    {
        printf("fail to open file");
        goto LEAVE;
    }
    write(ptn_tbl_fd, &flasherPtnTbl.entries[0], sizeof(NM_FLASHER_PTN_ENTRY) * NM_FLASHER_PTN_NUM_MAX);
    close(ptn_tbl_fd);

	DEBUG_PRINTF("inserting ecc to image\n");
    insert_ecc_to_image(flash_filename_no_ecc, flash_filename);
	DEBUG_PRINTF("inserting ecc done\n");
	
	//For some models, need to gen oob128
    if (TRUE == genOOB)
    {
        DEBUG_PRINTF("inserting oob128 ecc8 to image\n");
        insert_oob128_ecc8_to_image(flash_filename_no_ecc, flash_filename_2);
    }
    ret = 0;

LEAVE:
    free(tmpFileBuf);
    return ret;
}

static void show_usage(void)
{
    printf(
"Usage: make_flash [OPTIONS]\n\n"
"  -p, --partition=FILE             Partition file\n"
"  -o, --output=FILE                Output Filename prefix\n"
"  -c, --cloud=FILE			        Cloud Info file\n"
"  -b, --block=SIZE_KB              Block size of flash chip\n"
"  -s, --softver                    Software version in format \"V1.0.0P1\"\n"
"  -r, --release-time=FILE          Release time path\n"
"  -d, --debug                      Display debug information\n"
"  -h, --help                       Display this message\n"
"  -v, --version                    Display version\n"
"  -q, --genOOB                     Generate oob128 image\n"
    );
}

int main(int argc, char* argv[])
{
    int c = 0;
        
    static const struct option arg_options[] = {
        {"partition",		required_argument,	0, 'p'},
        {"output",			required_argument,	0, 'o'},
        {"blocksize",		required_argument,	0, 'b'},
        {"img_gen",			required_argument,	0, 'i'},
        {"debug",			no_argument,		0, 'd'},
        {"help",			no_argument,		0, 'h'},
        {"version",		    no_argument,		0, 'v'},
        {"manu",		    no_argument,		0, 'm'},
        {"genOOB",		    no_argument,		0, 'q'},
        {0, 0, 0, 0}
    };

    memset(&nmPtnStruct, 0, sizeof(NM_PTN_STRUCT));
    pNmPtnStruct = &nmPtnStruct;

    pFlasherPtnTbl = &flasherPtnTbl;
    
    /* get options */
    while (1) {
        int option_index = 0;
        c = getopt_long(argc, argv, "p:o:b:i:dhvmq", arg_options, &option_index);
        if (c == -1) break;
		
        switch (c) {
        case 'p':
            strncpy(pNmPtnStruct->ptnTblFile, optarg, 
                strlen(optarg) > NM_LONG_STRLEN?NM_LONG_STRLEN:strlen(optarg));
            break;
        case 'o':
            strncpy(pNmPtnStruct->prefix, optarg, 
                strlen(optarg) > NM_LONG_STRLEN?NM_LONG_STRLEN:strlen(optarg));
            break;
		case 'i':
			strncpy(g_imgGen, optarg, 
                strlen(optarg) > NM_LONG_STRLEN?NM_LONG_STRLEN:strlen(optarg));
            break;
        case 'b':
            sscanf(optarg, "%dKiB", &blockSizeKiB);
            blockSize = SIZE_KIB(blockSizeKiB);
            break;
        case 'd':
            debugOn = TRUE;
            break;
        case 'm':
            manuOn = TRUE;
            break;
        case 'q':
            genOOB = TRUE;
            break;
        case 'h':
            show_usage();
            exit(0);
            break;
        case 'v':
            printf("make_flash, version %s\n\n", VERSION);
            exit(0);
            break;
        }
    }
    DEBUG_PRINTF("make_flash: start.\n");
    if (blockSize == 0)
    {
        printf("No block size, -b, --blocksize=NUMKiB		Block Size in KiB\n");
        return -1;
    }
    
    if (parsePtnTblFile(pNmPtnStruct) < 0)
    {
        printf("parse partition file error.\n");
        return -1;
    }
    DEBUG_PRINTF("make_flash: parse partition table file done.\n");
    
    debug_printPtnTbl(pNmPtnStruct);

    if (chkPtnTblData(pNmPtnStruct) < 0)
    {
        printf("check partition table error.\n");
        return -1;
    }
    DEBUG_PRINTF("make_flash: check partition table done.\n");

	/*make fileNameStr */
	if (makeFileNameStr() != 0)
	{
		printf("make fileNameStr error.\n");
		return -1;
	}
	
    makeUpFile(FILE_UP);
    DEBUG_PRINTF("make_flash: make up file done.\n");
    
    if(manuOn)
    {
        DEBUG_PRINTF("manufacture is ON.\n");
        makeUpFile(FILE_MANU);
        DEBUG_PRINTF("make_flash: make Manu file done.\n");
    }

    makeFlashFile();
    DEBUG_PRINTF("make_flash: make flash file done.\n");
    
    return 0;
}
