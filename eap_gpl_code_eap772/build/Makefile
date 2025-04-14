help: $(PID)_help
	@exit 0

ifeq ($(shell ls -l /bin/sh | grep bash),)
$(error Please run 'sudo dpkg-reconfigure dash' and select 'NO' on you Ubuntu!)
endif
export GPL_BUILD=1
export CHIP ?= QCA
PLT ?= mips
cpu ?= be

include Makefile.flags

ifndef BOARD_TYPE
$(warning "Usage - make BOARD_TYPE=xxx PID=yyy WEB_PAGES_ID=zzz target(s)")
$(warning "BOARD_TYPE - $(shell ls product_configs/)")
$(warning "PID - eap fitap")
$(warning "WEB_PAGES_ID - VI_2.0 ...")
$(warning "Targets - PID_release PID_debug PID_develop ...")
$(error "Must specify BOARD_TYPE, see 'compile_howto.txt' for more information!")
endif

ifndef PID
$(warning "Usage - make BOARD_TYPE=xxx PID=yyy target(s)")
$(warning "BOARD_TYPE - $(shell ls product_configs/)")
$(warning "PID - eap fitap")
$(warning "Targets - PID_release PID_debug PID_develop ...")
$(error "You have to define product id, (PID=hipap).")
endif

ifneq ($(shell ls product_configs/$(BOARD_TYPE)/config.$(BOARD_TYPE)), product_configs/$(BOARD_TYPE)/config.$(BOARD_TYPE))
$(error "product_configs/$(BOARD_TYPE)/config.$(BOARD_TYPE) not found!")
endif

#add by fcs, for customize software
ifdef CUSTOM
CUSTOM_MK_FILE=product_configs/$(BOARD_TYPE)/spec/${CUSTOM}/custom.mk
ifeq ($(CUSTOM_MK_FILE), $(wildcard $(CUSTOM_MK_FILE)))
include $(CUSTOM_MK_FILE)
else
$(warning "$(CUSTOM_MK_FILE) not found!")
endif
endif

ifndef TOOLCHAIN_PACKAGE
$(error "Must specify TOOLCHAIN_PACKAGE value")
endif

ifndef KERNEL
$(error "Must specify KERNEL value")
endif

ifndef KERNELVER
$(error "Must specify KERNELVER value")
endif

ifndef KERNELARCH
$(error "Must specify KERNELARCH value")
endif

ifndef BUSYBOX
$(error "Must specify BUSYBOX value")
endif

######################## common env #####################################
export TOPDIR	=	$(shell pwd)/..
export IMAGE_INSTALL 		= $(TOPDIR)/images/$(PID)/$(BOARD_TYPE)
export IMAGE_BUILD_DIR		= $(IMAGE_INSTALL)/mkimage
export INSTALL_ROOT			= $(IMAGE_INSTALL)/rootfs
export TOOLCHAIN_PATH 		= $(TOPDIR)/toolchain
export TOOLS_PATH 			= $(TOPDIR)/tools
export ROOTFS_BUILD_PATH	= $(TOPDIR)/rootfs
export CONFIGDIR 			= $(TOPDIR)/build/product_configs/$(BOARD_TYPE)
export PRODUCT_SCRIPT_PATH	= $(TOPDIR)/build/product_configs/$(BOARD_TYPE)/script
export MKIMAGE_PAH 			= product_configs/$(BOARD_TYPE)/mkimage

export UTIL_PATH = $(TOOLCHAIN_PATH)/util

ifdef CONFIG_NM_NAND_TYPE
export FLASHMAKER = $(TOOLS_PATH)/flashMaker_nand
export GENSOFTVER = $(TOOLS_PATH)/gen_softver
else
export FLASHMAKER = $(TOOLS_PATH)/flashMaker
endif

export CROSSMAKE		=	$(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld RANLIB=$(TOOLPREFIX)ranlib
ifndef K_TOOLCHAIN_PREFIX
export ARCHMAKE			=	$(MAKE) ARCH=$(KERNELARCH) CROSS_COMPILE=$(TOOLPREFIX) 
else
export ARCHMAKE			=	$(MAKE) ARCH=$(KERNELARCH) CROSS_COMPILE=$(K_TOOLCHAIN_PREFIX) 
endif

export MODULESMAKE		=	$(ARCHMAKE) -C $(LINUX_KERNEL_PATH)

export LINUX_KERNEL_PATH 	=	$(TOPDIR)/linux/kernels/$(KERNEL)
export LINUX_MODULES_PATH	=	$(TOPDIR)/linux/modules
export KERNEL_CONFIG_FILE 	= 	$(CONFIGDIR)/linux.config
export KERNEL_CONFIG_TEMP 	= 	$(CONFIGDIR)/linux.config.tmp
export KERNLE_ENTRY			=	`$(TOOLPREFIX)readelf -h vmlinux|grep "Entry"|head -1|cut -d":" -f 2`
export KERNLE_LDADR			=	`$(TOOLPREFIX)readelf -S vmlinux|grep "] .text "|head -1|sed 's/[ ]\{2,\}/ /g'|cut -d" " -f6`
export LINUX_MODS_INSTALL 	=	$(INSTALL_ROOT)/lib/modules/$(KERNELVER)/kernel

export COMMON_LIB_SDK_PATH=$(TOPDIR)/sdk/com_lib/$(PID)
#########################################################################

######################## config env #####################################
ifeq ($(shell ls product_configs/$(BOARD_TYPE)/config), product_configs/$(BOARD_TYPE)/config)
CONFIG_OPTS:=$(shell cat product_configs/$(BOARD_TYPE)/config|sed '/^\#/d')
endif
######################## common env #####################################

###################################################################################
#  子函数 indir(dir, actions)
#  判断目录dir是否存在，如果存在则执行actions动作
#  否则调用打印提示，并以$(CHECK_STOP)为返回值结束make。
###################################################################################
export indir	=	if test -d $(1) && cd $(1) ; then $(2); \
								else echo "Directory '$(strip $(1))' not exists." && exit $(CHECK_STOP); fi

###################################################################################
#  函数 makemod(subdirs, targets)
#  编译subdirs目录下的内核模块，
#  install时拷贝targets到目标$(INSTALL_ROOT)/lib/modules/$(KERNELVER)/kernel/
###################################################################################

export makemod = if [ "$*" = "install" ]; then \
							if [ "$(shell ls $(strip $(1))/$(strip $(2)) 2>/dev/null)" != "" ]; then \
							cp -f $(strip $(1))/$(strip $(2)) $(if $(3), $(3), $(LINUX_MODS_INSTALL)); fi \
							else $(MODULESMAKE) $* M=$(strip $(1)) SUBDIRS=$(strip $(1)); fi
###################################################################################
CONFIG_CHECK=$(TOPDIR)/tools/config_check/config_check

config_check:
	$(CONFIG_CHECK) -d -p $(TOPDIR)/build/product_configs/$(BOARD_TYPE)

ifeq ($(CHIP), QCA)
include qca/Makefile
else ifeq ($(CHIP), BCM)
include bcm/Makefile
else ifeq ($(CHIP), MTK)
include mtk/Makefile
endif
ifndef GPL_BUILD
include Makefile.tools
endif
######################################### menuconfig #######################################
menuconfig:
	@echo "make menuconfig... ...";
	cp $(TOPDIR)/build/product_configs/$(BOARD_TYPE)/config $(TOPDIR)/build/.config
	$(TOPDIR)/build/menu/scripts/config/mconf $(TOPDIR)/build/menu/sysdeps/linux/Config.in 
	cp $(TOPDIR)/build/.config $(TOPDIR)/build/product_configs/$(BOARD_TYPE)/config

######################################### common rule #######################################

%_rebuild: %_clean %_build %_install
	@echo "rebuild <$@> at `date`."

%: %_build %_install
	@echo "built and installed <$@> at `date`."

.PHONY: all clean toolchain
.PHONY: everything_config everything_prep everything_build everything_clean everything_install

