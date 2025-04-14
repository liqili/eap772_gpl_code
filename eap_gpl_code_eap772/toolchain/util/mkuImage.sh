#!/bin/sh -x

#
# $1 == u-boot/tools path
# $2 == kernel tree path
# $3 == optional additions to filename

MKIMAGE=$1/mkimage
VMLINUX=$2/vmlinux
VMLINUXBIN=$2/vmlinux.bin
# NOTE You can direct the outputs elsewhere by pre-defining TFTPPATH
#if [ -z "$TFTPPATH" ]
#then
#    TFTPPATH=/tftpboot/`whoami`
#fi
#echo $0 Using TFTPPATH=$TFTPPATH  ###### DEBUG

ENTRY=`${TOOLPREFIX}readelf -h ${VMLINUX}|grep "Entry"|head -1|cut -d":" -f 2`
LDADDR=`${TOOLPREFIX}readelf -S ${VMLINUX}|grep "] .text "|head -1|sed 's/[ ]\{2,\}/ /g' | cut -d" " -f6`
       
# gzip -f ${VMLINUXBIN}

if [ $# -gt 3 ]
then
	suffix=$3
else
	suffix=
fi

if [ "xx$4" = "xxlzma" -o "xx$3" = "xxlzma" ]
then
	echo "**** Generating vmlinux${suffix}.lzma.uImage ********"
	lzma e ${VMLINUXBIN} ${VMLINUXBIN}.lzma
	${MKIMAGE} \
		-A mips -O linux -T kernel -C lzma \
		-a 0x${LDADDR} -e ${ENTRY} -n "Linux Kernel Image" \
		-d ${VMLINUXBIN}.lzma ${IMAGE_INSTALL}/vmlinux${suffix}.lzma.uImage
else
	echo "**** Generating vmlinux${suffix}.gz.uImage ********"
	gzip -9c ${VMLINUXBIN} > ${VMLINUXBIN}.gz 
	${MKIMAGE} \
		-A mips -O linux -T kernel -C gzip \
		-a 0x${LDADDR} -e ${ENTRY} -n "Linux Kernel Image" \
		-d ${VMLINUXBIN}.gz ${IMAGE_INSTALL}/vmlinux${suffix}.gz.uImage
fi
