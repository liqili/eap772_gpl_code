/*! Copyright(c) 2014 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     make_flash.h
 *\brief    Tool for generating firmware
 *
 *\author   Huang Qinglou
 *\version  1.0.0
 *\date     18/Aug/2014
 *
 *\history \arg 
 *\01   18/Aug/2014  HQL     Create.
 *\02	05/Feb/2015	 Liwei   add CloudInfo.
 */



#ifndef _MAKE_FLASH_H_
#define _MAKE_FLASH_H_

#include <stdint.h>

#define VERSION			"1.0"
#define EXTRA_PARA_NAME	"extra-para" 
#define OS_IMAGE_NAME	"os-image"
#define PRODUCT_INFO_NAME	"product-info"
#define HISTORY_NAME	"softver-fw_id"
#define RELEASE_TIME_FILE	"release_time"

#define NM_PTN_TABLE_SIZE		(0x800)/*2K*/
#define NM_PTN_NUM_MAX			(40)
#define NM_FLASHER_PTN_NUM_MAX	(32)
#define NM_PTN_FILE_PATH		(512)
#define NM_NORMAL_STRLEN		(256)
#define NM_LONG_STRLEN			(512)
#define NM_PTN_NAME_LEN 		(32)
#define UP_PTNTBL_SIZE      	(0x800)

#define BUF_LEN_2M		(2 * 1024 * 1024)
#define BUF_LEN_4M		(4 * 1024 * 1024)
#define BUF_LEN_8M		(8 * 1024 * 1024)
#define BUF_LEN_16M		(16 * 1024 * 1024)
#define BUF_LEN_32M		(32 * 1024 * 1024)
#define BUF_LEN_128M	(128 * 1024 * 1024)


#define NM_BYTE_LEN             4
#define NM_FILE_EMPTY           "#"
#define NM_FILE_NOOUTPUT        "*"

#define TRUE	(1)
#define FALSE	(0)

#define CLOUD_HEAD_OFFSET 	252
#define HW_ID_LEN			16
#define FW_ID_LEN			16
#define OEM_ID_LEN			16
#define STR_ID_LEN			32
#define HW_LIST_MAX		128
#define INFO_LEN			20

#define CLOUD			"Cloud"
#define CLOUD_HWID		"hw_id"
#define CLOUD_OEMID		"oem_id"
#define CLOUD_HWLIST	"hw_list"
#define FWTYPE			"fw-type"
#define SOFTVERSION		"soft_ver"
#define CONFIGVERSION	"cfg_ver"

#define HEADER_VERSION 	0x00000100
#define MAGIC_LEN 		20
#define CRC_LEN 			16
#define RSA_SIGN_LEN 		128
#define FWID_FL_MASK_LEN 	12
#define FW_DESC_LEN	 	12

/* for config version info */
#define MAX_REPOS_PATH_LEN		(128)
#define MAX_PRODUCT_NAME_LEN	(64)
#define MAX_HW_VERSION_LEN		(16)
#define MAX_CODE_VERSION_LEN	(64)

#define FW_TYPE_FLASH		(0x1)
#define FW_TYPE_UP			(0x2)
#define FW_TYPE_MANU		(0x4)
#define FW_TYPE_ENABLED(type, mask)		(type & mask)

#define PTN_SOFT_VERSION_NAME "soft-version"

typedef enum flash_em_size{
	FLASH_EM_2M  = 2,
	FLASH_EM_4M  = 4,
	FLASH_EM_8M  = 8,
	FLASH_EM_16M = 16,
	FLASH_EM_32M = 32,
	FLASH_EM_128M = 128
}FLASH_EM_SIZE;

typedef enum upfile_type{
    FILE_UP  = 0,
	FILE_MANU,
    FILE_END
}UP_FILE_TYPE;

typedef enum nm_ptncontent_type
{
	NM_PTN_CONTENT_MTD = 0,
	NM_PTN_CONTENT_FW,
	NM_PTN_CONTENT_MANU,
	NM_PTN_CONTENT_USER
}NM_PTN_CONTENT_TYPE;

typedef enum nm_root_type
{
	NM_ROOT_NONE = 0,
	NM_ROOT_UP,
	NM_ROOT_BAK
}NM_ROOT_TYPE;

typedef enum nm_type
{
	NM_TYPE_BOTH = 0,
	NM_TYPE_FLASH,
	NM_TYPE_UP,
	NM_TYPE_NOT,
	NM_TYPE_MANU,
	NM_TYPE_FLASHMANU,
	NM_TYPE_ALL,
	NM_TYPE_END
}NM_TYPE;

typedef enum nm_offset
{
	NM_OFFSET_NO = 0,
	NM_OFFSET_4B,
	NM_OFFSET_8B,
	NM_OFFSET_12B,
	NM_OFFSET_END
}NM_OFFSET;

typedef struct soft_ver_struct
{
	uint32_t sn;
	uint32_t buildDate;
	uint32_t releaseTime;
	uint32_t additionalHardVer;
#ifdef CONFIG_BOOT_VERSION_SUPPORT
	uint32_t bootVer;
#endif /* CONFIG_BOOT_VERSION_SUPPORT */
}SOFT_VER_STRUCT;

typedef struct _CONFIG_VER_STRUCT
{
	unsigned char product_name[MAX_PRODUCT_NAME_LEN];
	unsigned char hw_ver[MAX_HW_VERSION_LEN];		/* hardware version, from product-info */
	unsigned char repos_path[MAX_REPOS_PATH_LEN];
	unsigned char soft_ver[MAX_CODE_VERSION_LEN]; 	/* code (svn or git) version */
} CONFIG_VER_STRUCT;


//add by zengwei below for factory write syspara
typedef struct extra_para_struct
{
	unsigned char dbootFlag;	
	unsigned char integerFlag;	
}EXTRA_PARA_STRUCT;
//add by zengwei above for factory write syspara

typedef struct nm_flasher_ptn_entry
{
	unsigned int baseBlock;
	unsigned int endBlock;
	unsigned int actualBlocks;
	unsigned int reserved;
}NM_FLASHER_PTN_ENTRY;

typedef struct nm_partition_entry
{
	char name[NM_PTN_NAME_LEN];
	char filePath[NM_PTN_FILE_PATH];/*input file path*/
	unsigned int base;
	unsigned int baseBlock; /* block this partition start. */
	int size;
	int actualSize;
	int actualSizePadded;
	int blocks;/* blocks this partition used. */
	int actualBlocks; /* blocks this partition file actually used. */
	NM_PTN_CONTENT_TYPE contentType;
	int writeCap;/* if the partition can be read/erase/write by nvrammananger */
	int rootType;
	int writeType; /* which kind of firmware contains this part */
	unsigned int usedFlag;	
} NM_PTN_ENTRY;

typedef struct nm_partition_list
{
	NM_PTN_ENTRY entries[NM_PTN_NUM_MAX];
	char ptnTblFile[NM_LONG_STRLEN];/*分 区 表 配 置 信 息 文 件 */
	char prefix[NM_LONG_STRLEN];
	char productinfo[NM_LONG_STRLEN];
	SOFT_VER_STRUCT softVer;
	int ptnNum;
	int flashSize;
} NM_PTN_STRUCT;

typedef struct nm_up_tbl_entry
{
	char name[NM_PTN_NAME_LEN];
	int base;/*base from up content head. */
	int size;
	int content_type;
}NM_UP_PTN_ENTRY;

typedef struct nm_up_ptn_tbl
{
	int rootSize;
	int ptnNum;
	int isManu;
	NM_UP_PTN_ENTRY entries[NM_PTN_NUM_MAX];
}NM_UP_PTN_TBL_STRUCT;

/* partition table for flash programmer */
typedef struct nm_flash_ptn_list
{
	NM_FLASHER_PTN_ENTRY entries[NM_FLASHER_PTN_NUM_MAX];
	char ptnTblFile[NM_LONG_STRLEN];
	char prefix[NM_LONG_STRLEN];
	int ptnNum;
} NM_FLASHER_PTN_STRUCT;

/* fwup-file header */
#define FWUP_HDR_MD5_LEN 		16
#define FWUP_HDR_PRODUCT_ID_LEN 0x1000

typedef struct fwup_file_head
{
	uint32_t fileSize;
	char fileMd5[FWUP_HDR_MD5_LEN];
	char modelId[FWUP_HDR_PRODUCT_ID_LEN];
} FWUP_FILE_HEAD;


typedef struct _UPGRADE_HEADER
{
	unsigned int headerVersion;
	unsigned char magicNumber[MAGIC_LEN];
	unsigned short tagLength;
	unsigned short vendorId;
	unsigned short zoneCode;
	unsigned short contentTypes;
	unsigned char rsaSignature[RSA_SIGN_LEN];
	unsigned short hwIdNum;
	unsigned short fwIdFLNum;
	unsigned char fwIdFLMask[FWID_FL_MASK_LEN];
	unsigned char hwIdList[0][HW_ID_LEN];
//	unsigned char fwIdFL[0][FW_ID_LEN];
} UPGRADE_HEADER;

typedef struct _TP_HEADER
{
	unsigned int headerVersion;
	unsigned char magicNumber[MAGIC_LEN];

	unsigned int kernelLoadAddress;
	unsigned int kernelEntryPoint;

	unsigned short vendorId;
	unsigned short zoneCode;

	unsigned int partitionNum;
	unsigned int factoryBootOffset;
	unsigned int factoryBootLen;
	unsigned int factoryInfoOffset;
	unsigned int factoryInfoLen;
	unsigned int radioOffset;
	unsigned int radioLen;
	unsigned int ucOffset;
	unsigned int ucLen;
	unsigned int bootloaderOffset;
	unsigned int bootloaderLen;
	unsigned int tpHeaderOffset;
	unsigned int tpHeaderLen;
	unsigned int kernelOffset;
	unsigned int kernelLen;
	unsigned int romFsOffset;
	unsigned int romFsLen;
	unsigned int jffs2FsOffset;
	unsigned int jffs2FsLen;

	unsigned char factoryInfoCRC[CRC_LEN];
	unsigned char radioCRC[CRC_LEN];
	unsigned char ubootCRC[CRC_LEN];
	unsigned char kernelAndRomfsCRC[CRC_LEN];
	
	unsigned char fwId[FW_ID_LEN];
	unsigned char fwDescription[FW_DESC_LEN];
	unsigned int fwIdBLNum;
	unsigned char fwIdBL[0][FW_ID_LEN];
} TP_HEADER;

#endif

