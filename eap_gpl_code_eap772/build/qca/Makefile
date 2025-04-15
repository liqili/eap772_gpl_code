
# 此处定义所有QCA机型的SDK版本,生成规则见Makefile.flags中对CONFIG_TP_QCA_SDK_VERSION_CODE的介绍
export TP_SPF_0_9_5=0x905
export TP_SPF_0_10_2=0xA02
export TP_SPF_2_0_0=0x20000
export TP_SPF_5_0_2=0x50002
export TP_SPF_10_0_0=0xA0000
export TP_SPF_11_1_0=0xB0100
export TP_SPF_11_3_0=0xB0300
export TP_SPF_11_4_0=0xB0400
export TP_SPF_12_1_0=0xC0100
export TP_SPF_12_2_0=0xC0200
export TP_SPF_12_4_0=0xC0400

########################## toolchain and utils used for cross build #################
export TOOLCHAIN_INSTALL	= $(TOPDIR)/build/toolchain
export LIBC_PATH = $(TOOLCHAIN_INSTALL)/$(TOOLCHAIN_PACKAGE)/$(TOOLARCH)/..
export STAGING_DIR = $(LIBC_PATH)/..
export MKIMG_PATH = $(TOPDIR)/build/product_configs/$(BOARD_TYPE)/mkimage
export RUNTIME_DATA_DIR_NAME = runtime_data_dir
export FACTORY_DATA_DIR_NAME = factory_data
export FACTORY_BACKUP_DIR_NAME = backup_data
export RUNTIME_DATA_DIR = $(IMAGE_BUILD_DIR)/$(RUNTIME_DATA_DIR_NAME)
export FACTORY_DATA_DIR = $(IMAGE_BUILD_DIR)/$(FACTORY_DATA_DIR_NAME)
export FACTORY_BACKUP_DIR = $(IMAGE_BUILD_DIR)/$(FACTORY_BACKUP_DIR_NAME)
ifneq ($(findstring $(CONFIG_TP_QCA_SDK_VERSION_CODE),$(TP_SPF_11_1_0) $(TP_SPF_11_3_0) $(TP_SPF_11_4_0) $(TP_SPF_12_1_0) $(TP_SPF_12_2_0) $(TP_SPF_12_4_0)), )
export RUNTIME_BACKUP_DIR = $(IMAGE_BUILD_DIR)/runtime_back_dir
endif

ifndef TOOLARCH
$(error "Must specify TOOLARCH value")
endif


ifeq ($(shell getconf LONG_BIT), 64)
IMG_GEN = $(TOOLS_PATH)/img_gen_64bit
else
IMG_GEN = $(TOOLS_PATH)/img_gen_32bit
endif

ifeq ($(IMG_GEN_OOB128), 1)
FLASH_MAKE_EXOPT=-q
endif

# Save PATH for later use for compiling host-only tools etc.
export ORIGINAL_PATH:=$(PATH)
export PATH:=$(UTIL_PATH):$(LIBC_PATH)/usr/bin:$(LIBC_PATH)/bin:`pwd`:${PATH}
ifeq ($(KERNELVER), 3.3.8)
export MKSQUASHFS ?= mksquashfs42
export comp_option = -comp xz
else ifeq ($(KERNELVER), 4.4.60)
export MKSQUASHFS ?= mksquashfs42
export comp_option = -comp xz
else ifeq ($(KERNELVER), 5.4.164)
export MKSQUASHFS ?= mksquashfs42
export comp_option = -comp xz
else ifeq ($(KERNELVER), 5.4.213)
export MKSQUASHFS ?= mksquashfs42
export comp_option = -comp xz
else
export MKSQUASHFS ?= mksquashfs4
export comp_option =
endif

NAND_APPS =
ifdef CONFIG_NM_NAND_TYPE
ifndef GPL_BUILD
NAND_APPS += mtd-utils nand_flash_maker
ifneq ($(INCLUDE_CPU_QCA_IPQ957X)$(INCLUDE_CPU_QCA_IPQ53XX), y)
export APPS_ADDITIONAL += ubi-utils
export APPS_ADDITIONAL += fw_setenv
endif

else
NAND_APPS += mtd-utils
endif
#NAND_APPS += mtd-utils
#export APPS_ADDITIONAL += ubi-utils
#ifndef GPL_BUILD
#NAND_APPS += nand_flash_maker
#export APPS_ADDITIONAL +=fw_setenv
#endif

endif

export fakeroot_package       = fakeroot-1.9.1
export squashfs4_package      = squashfs4.0-lzma
export squashfs42_package     = squashfs-4.2
export lzma_package           = lzma457
export mkimage_package        = mkimage
export xz_package             = xz-5.2.1
export padjffs2_package       = padjffs2
export patchelf_package       = patchelf-0.9
export sstrip_package 		  = sstrip
export gcc_package            = $(TOOLCHAIN_PACKAGE)
export mtd-utils_package      = mtd-utils-1.5.1

ifdef CONFIG_NM_NAND_TYPE
export ubi-utils_path   = $(APPS_PATH)/ubi-utils-1.5.1
export fw_setenv_path   = $(TOPDIR)/sdk/qca/u-boot/u-boot-src/u-boot-2016
endif

RSTRIP=export CROSS="$(TOOLPREFIX)" KEEP_BUILD_ID=1 NO_RENAME=1 ; \
	   NM="$(TOOLPREFIX)nm" \
	   STRIP="$(UTIL_PATH)/sstrip" \
	   STRIP_KMOD="$(PRODUCT_SCRIPT_PATH)/strip-kmod.sh" \
	   PATCHELF="$(UTIL_PATH)/patchelf" \
	   $(PRODUCT_SCRIPT_PATH)/rstrip.sh

padjffs2:
	@cd $(TOOLCHAIN_PATH) && \
	rm -rf $(padjffs2_package) && \
	tar -xf $(padjffs2_package).tar.bz2 && \
	cd $(padjffs2_package) && make all && cp padjffs2 $(UTIL_PATH)/padjffs2

xz:
	@cd $(TOOLCHAIN_PATH) && \
	rm -rf $(xz_package) && \
	tar -xf $(xz_package).tar.bz2 && \
	cd $(xz_package) && ./configure --enable-small --enable-assume-ram=4 --disable-assembler --disable-werror && \
	make -C src/liblzma

sstrip:
	@cd $(TOOLCHAIN_PATH) && \
		cd $(sstrip_package) && make all && cp sstrip $(UTIL_PATH)/

patchelf:
	@cd $(TOOLCHAIN_PATH) && \
	rm -rf $(patchelf_package) && \
	tar -xf $(patchelf_package).tar.bz2 && \
	cd $(patchelf_package) && ./configure && make && \
	cp src/patchelf $(UTIL_PATH)/patchelf

squashfs4:
	@cd $(TOOLCHAIN_PATH) && \
		rm -rf $(squashfs4_package) && \
		tar -xf $(squashfs4_package).tar.gz && \
		cd $(squashfs4_package)/squashfs-tools && \
		make && cp mksquashfs $(UTIL_PATH)/mksquashfs4

squashfs42:
	@cd $(TOOLCHAIN_PATH) && \
		rm -rf $(squashfs42_package) && \
		tar -xf $(squashfs42_package).tar.bz2 && \
		cd $(squashfs42_package) && make XZ_SUPPORT=1 LZMA_XZ_SUPPORT=1 XATTR_SUPPORT= LZMA_LIB=$(TOOLCHAIN_PATH)/$(xz_package)/src/liblzma/.libs/liblzma.a -I. mksquashfs && \
		cp mksquashfs $(UTIL_PATH)/mksquashfs42

fakeroot:
	@cd $(TOOLCHAIN_PATH) && \
		rm -rf $(fakeroot_package) && \
		tar -xf $(fakeroot_package).tar.gz && \
		cd $(fakeroot_package) && \
		sed -i 's/        SHELL=/        SHELL=\/bin\/bash #/' ./configure &&\
		./configure --prefix=/usr --disable-nls TOOLPREFIX= STRIP= &&\
		make && sed \
		-e 's,^PREFIX=.*,PREFIX=$(TOOLCHAIN_PATH)/$(fakeroot_package),g' \
		-e 's,^BINDIR=.*,BINDIR=$(TOOLCHAIN_PATH)/$(fakeroot_package),g' \
		-e 's,^PATHS=.*,PATHS=$(TOOLCHAIN_PATH)/$(fakeroot_package)/.libs:/lib:/usr/lib,g' \
		scripts/fakeroot > $(UTIL_PATH)/fakeroot && \
		chmod u+x $(UTIL_PATH)/fakeroot

lzma:
	@which g++ || echo "No g++ compiler found on you PC.";
	@if which g++; then cd $(TOOLCHAIN_PATH) && \
		rm -rf $(lzma_package) && \
		mkdir -p $(lzma_package) && \
		tar -xf $(lzma_package).tar.bz2 -C $(lzma_package) && \
		cd $(lzma_package)/CPP/7zip/Compress/LZMA_Alone && \
		make -f makefile.gcc && \
		cp -f lzma $(UTIL_PATH); fi

mkimage:
	@cd $(TOOLCHAIN_PATH) && \
		rm -rf $(mkimage_package) && \
		tar -xf $(mkimage_package).tar.gz && \
		cd $(mkimage_package) && make defconfig  HOSTCFLAGS="-O2 -I/usr/include/x86_64-linux-gnu" HOSTLDFLAGS="-L/lib/x86_64-linux-gnu" && make tools-only  HOSTCFLAGS="-O2 -I/usr/include/x86_64-linux-gnu" HOSTLDFLAGS="-L/lib/x86_64-linux-gnu" && \
		cp -f tools/mkimage $(UTIL_PATH)

mtd-utils:
	cd $(TOOLCHAIN_PATH) && \
		rm -rf $(mtd-utils_package) && \
		tar -xf $(mtd-utils_package).tar.gz && \
		cd $(mtd-utils_package) && make WITHOUT_XATTR=1 WITHOUT_LZO=1 WITHOUT_XZ=1 && \
		cp -f ubi-utils/ubinize $(UTIL_PATH) && \
		cp -f mkfs.ubifs/mkfs.ubifs $(UTIL_PATH)

nand_flash_maker:
	cd $(TOOLCHAIN_PATH)/make_flash_src_nand && make clean && make && \
		cp -f make_flash $(TOOLS_PATH)/flashMaker_nand && \
		cp -f gen_softver $(TOOLS_PATH) && \
		cp -f img_gen $(IMG_GEN)

util:xz lzma padjffs2 fakeroot squashfs4 squashfs42 mkimage patchelf sstrip $(VENDOR_UTIL) $(NAND_APPS)
	@echo make $^ done.
	@chmod +x $(UTIL_PATH)/*

toolchain_clean:
	@rm -rf $(TOOLCHAIN_INSTALL)/$(gcc_package)

toolchain:
	@echo Uncompressing toolchain, please wait...
	@mkdir -p $(TOOLCHAIN_INSTALL)
	@[ -f $(TOOLCHAIN_PATH)/$(gcc_package).tar.bz2 ] || cat $(TOOLCHAIN_PATH)/$(gcc_package).tar.bz2_aa $(TOOLCHAIN_PATH)/$(gcc_package).tar.bz2_ab $(TOOLCHAIN_PATH)/$(gcc_package).tar.bz2_ac > $(TOOLCHAIN_PATH)/$(gcc_package).tar.bz2
	@tar -jxf $(TOOLCHAIN_PATH)/$(gcc_package).tar.bz2 -C $(TOOLCHAIN_INSTALL)
	@test -z "$(TOOLCHAIN_POSTBUILD)" || eval "$(TOOLCHAIN_POSTBUILD)"


ubi-utils_clean:
	@$(call indir, $(ubi-utils_path), $(CROSSMAKE) clean);

ubi-utils_build:
	@$(call indir, $(ubi-utils_path), $(CROSSMAKE));

ubi-utils_install:
	$(call indir, $(ubi-utils_path)/ubi-utils,  $(TOOLPREFIX)strip ubiattach);
	$(call indir, $(ubi-utils_path)/ubi-utils,  $(TOOLPREFIX)strip ubiblock);
	$(call indir, $(ubi-utils_path)/ubi-utils,  $(TOOLPREFIX)strip ubicrc32);
	$(call indir, $(ubi-utils_path)/ubi-utils,  $(TOOLPREFIX)strip ubidetach);
	$(call indir, $(ubi-utils_path)/ubi-utils,  $(TOOLPREFIX)strip ubiformat);
	$(call indir, $(ubi-utils_path)/ubi-utils,  $(TOOLPREFIX)strip ubimkvol);
	$(call indir, $(ubi-utils_path)/ubi-utils,  $(TOOLPREFIX)strip ubinfo);
	$(call indir, $(ubi-utils_path)/ubi-utils,  cp -f ubiattach $(APPS_INSTALL));
	$(call indir, $(ubi-utils_path)/ubi-utils,  cp -f ubiblock $(APPS_INSTALL));
	$(call indir, $(ubi-utils_path)/ubi-utils,  cp -f ubicrc32 $(APPS_INSTALL));
	$(call indir, $(ubi-utils_path)/ubi-utils,  cp -f ubidetach $(APPS_INSTALL));
	$(call indir, $(ubi-utils_path)/ubi-utils,  cp -f ubiformat $(APPS_INSTALL));
	$(call indir, $(ubi-utils_path)/ubi-utils,  cp -f ubimkvol $(APPS_INSTALL));
	$(call indir, $(ubi-utils_path)/ubi-utils,  cp -f ubinfo $(APPS_INSTALL));

fw_setenv_clean:
	$(call indir, $(fw_setenv_path), $(CROSSMAKE) clean);

fw_setenv_build:
ifeq ($(INCLUDE_CPU_QCA_IPQ807X), y)
	$(call indir, $(fw_setenv_path), make ipq807x_defconfig)
else ifeq ($(INCLUDE_CPU_QCA_IPQ957X), y)
	$(call indir, $(fw_setenv_path), make ipq9574_defconfig)
else ifeq ($(INCLUDE_CPU_QCA_IPQ53XX), y)
	$(call indir, $(fw_setenv_path), make ipq5332_defconfig)
else ifeq ($(INCLUDE_CPU_QCA_IPQ60XX), y)
	$(call indir, $(fw_setenv_path), make ipq6018_defconfig)
endif
	$(call indir, $(fw_setenv_path), $(CROSSMAKE) HOSTCFLAGS="-DDO_DEPS_ONLY" STRIP=$(TOOLPREFIX)strip env);

fw_setenv_install:
	$(call indir, $(fw_setenv_path)/tools/env,  cp -f fw_printenv $(APPS_INSTALL)/fw_setenv && cd $(APPS_INSTALL) && ln -sf fw_setenv fw_printenv);


################################# linux modules ####################################

ifeq ($(KERNELVER), 3.3.8)
export BOARD_KERNEL_MODULES = netfilter netsched $(MODS_ADDITIONAL)
else
export BOARD_KERNEL_MODULES = netfilter netsched $(MODS_ADDITIONAL)
endif
ifeq ($(strip ${CONFIG_MTD_OOPS_LOG}),y)
BOARD_KERNEL_MODULES+= mtdoops
endif

ifeq ($(strip ${CONFIG_WLAN_CFG80211}),y)
BOARD_KERNEL_MODULES+= cfg80211
endif

ifeq ($(strip ${CONFIG_NM_NAND_TYPE}),y)
BOARD_KERNEL_MODULES+= bootconfig
endif

ifeq ($(strip ${CONFIG_USB_SUPPORT}),y)
BOARD_KERNEL_MODULES+= scsi_mod
BOARD_KERNEL_MODULES+= sd_mod
BOARD_KERNEL_MODULES+= usb-common
BOARD_KERNEL_MODULES+= usbcore
BOARD_KERNEL_MODULES+= usblp
BOARD_KERNEL_MODULES+= usb-storage
BOARD_KERNEL_MODULES+= udc-core
BOARD_KERNEL_MODULES+= xhci-hcd
BOARD_KERNEL_MODULES+= xhci-pci
BOARD_KERNEL_MODULES+= xhci-plat-hcd
BOARD_KERNEL_MODULES+= uas
BOARD_KERNEL_MODULES+= libcomposite
BOARD_KERNEL_MODULES+= dwc3
BOARD_KERNEL_MODULES+= dwc3-qcom
BOARD_KERNEL_MODULES+= g_diag
BOARD_KERNEL_MODULES+= usb_f_diag
BOARD_KERNEL_MODULES+= phy-qcom-qusb2
BOARD_KERNEL_MODULES+= ntfs
BOARD_KERNEL_MODULES+= fat
BOARD_KERNEL_MODULES+= vfat
BOARD_KERNEL_MODULES+= nls_cp437
BOARD_KERNEL_MODULES+= nls_iso8859-1
BOARD_KERNEL_MODULES+= nls_utf8
BOARD_KERNEL_MODULES+= msdos
BOARD_KERNEL_MODULES+= fuse
ifneq ($(findstring $(CONFIG_TP_QCA_SDK_VERSION_CODE),$(TP_SPF_10_0_0) $(TP_SPF_11_1_0) $(TP_SPF_11_3_0) $(TP_SPF_11_4_0) $(TP_SPF_12_1_0) $(TP_SPF_12_2_0) $(TP_SPF_12_4_0)), )
BOARD_KERNEL_MODULES+= diagchar
BOARD_KERNEL_MODULES+= usb_common
BOARD_KERNEL_MODULES+= crc-ccitt
endif
else
ifneq ($(findstring $(CONFIG_TP_QCA_SDK_VERSION_CODE),$(TP_SPF_10_0_0) $(TP_SPF_11_1_0) $(TP_SPF_11_3_0) $(TP_SPF_11_4_0) $(TP_SPF_12_1_0) $(TP_SPF_12_2_0) $(TP_SPF_12_4_0)), )
BOARD_KERNEL_MODULES+= diagchar
BOARD_KERNEL_MODULES+= usb_f_diag
BOARD_KERNEL_MODULES+= libcomposite
BOARD_KERNEL_MODULES+= usb_common
BOARD_KERNEL_MODULES+= crc-ccitt
BOARD_KERNEL_MODULES+= udc-core
endif
endif
############################## modules common rules ################################
ifneq ($(findstring $(CONFIG_TP_QCA_SDK_VERSION_CODE),$(TP_SPF_12_1_0) $(TP_SPF_12_2_0) $(TP_SPF_12_4_0)), )
modules_all:
	$(MODULESMAKE) modules
else 
modules_all:
	@echo "do nothing"
endif


modules_clean:$(addsuffix _clean,  $(filter-out $(MODS_EXCLUDE), $(BOARD_KERNEL_MODULES)))
	@echo make $@ at `date`

ifneq ($(findstring $(CONFIG_TP_QCA_SDK_VERSION_CODE),$(TP_SPF_12_1_0) $(TP_SPF_12_2_0) $(TP_SPF_12_4_0)), )
modules_build: modules_all
	@echo make $@ at `date`
else
modules_build: $(addsuffix _modules, $(filter-out $(MODS_EXCLUDE), $(BOARD_KERNEL_MODULES)))
	@echo make $@ at `date`
endif

modules_install: $(addsuffix _install, $(filter-out $(MODS_EXCLUDE), $(BOARD_KERNEL_MODULES)))
	@echo make $^
	@echo make $@ at `date`

############################# linux modules build rules ############################

netfilter_%:
	@if [[ "$(KERNELVER)" = "2.6.31" ]] || [[ "$(KERNELVER)" = "2.6.36" ]] || [[ "$(KERNELVER)" = "3.3.8" ]] || [[ "$(KERNELVER)" = "4.4.60" ]] || [[ "$(KERNELVER)" = "5.4.164" ]] || [[ "$(KERNELVER)" = "5.4.213" ]]; then \
		$(call makemod, $(LINUX_KERNEL_PATH)/net/netfilter, *.ko); \
	fi;
	@$(call makemod, $(LINUX_KERNEL_PATH)/net/ipv4/netfilter, *.ko);
	@$(call makemod, $(LINUX_KERNEL_PATH)/net/bridge/netfilter, *.ko);
	@if [[ "$(KERNELVER)" = "5.4.164" ]] || [[ "$(KERNELVER)" = "5.4.213" ]]; then \
		$(call makemod, $(LINUX_KERNEL_PATH)/net/ipv6/netfilter, nf_defrag_ipv6.ko); \
	fi;

netconsole_%:
	@$(call makemod, $(LINUX_KERNEL_PATH)/drivers/net, netconsole.ko);

netsched_%:
	@$(call makemod, $(LINUX_KERNEL_PATH)/net/sched, *.ko);

mtdoops_%:
	@$(call makemod, $(LINUX_KERNEL_PATH)/drivers/mtd, *.ko);


ifeq ($(strip ${CONFIG_NM_NAND_TYPE}),y)
bootconfig_%:
	@$(call makemod, $(LINUX_KERNEL_PATH)/drivers/platform/ipq, *.ko);
endif

cfg80211_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/net/wireless, cfg80211.ko);
	
bonding_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/net/bonding/, bonding.ko);

libcrc32c_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/lib/, libcrc32c.ko);

slhc_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/net/slip/, slhc.ko);

ppp_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/net/ppp/, *.ko);

usb-common_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/common, usb-common.ko);

usb-storage_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/storage, usb-storage.ko);

uas_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/storage, uas.ko);
	
usbcore_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/core, usbcore.ko);	
	
g_diag_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget/legacy, g_diag.ko);	

scsi_mod_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/scsi, scsi_mod.ko);

sd_mod_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/scsi, sd_mod.ko);
	
usblp_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/class, usblp.ko);

xhci-hcd_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/host, xhci-hcd.ko);

xhci-pci_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/host, xhci-pci.ko);

xhci-plat-hcd_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/host, xhci-plat-hcd.ko);	
	
dwc3_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/dwc3, dwc3.ko);

dwc3-qcom_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/dwc3, dwc3-qcom.ko);
	
phy-qcom-qusb2_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/phy/qualcomm, phy-qcom-qusb2.ko);

ntfs_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/fs/ntfs, ntfs.ko);

fat_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/fs/fat, fat.ko);

vfat_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/fs/fat, vfat.ko);
	
msdos_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/fs/fat, msdos.ko);
	
nls_utf8_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/fs/nls, nls_utf8.ko);	

nls_cp437_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/fs/nls, nls_cp437.ko);

nls_iso8859-1_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/fs/nls, nls_iso8859-1.ko);

	
fuse_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/fs/fuse, fuse.ko);
	
diagchar_%:
	@if [ "$*" = "install" ]; then \
		if [ -d $(SDK_FTM_INSTALL) ]; then \
		cp $(LINUX_KERNEL_PATH)/drivers/char/diag/diagchar.ko $(SDK_FTM_INSTALL); \
		fi;\
		if [[ "$(KERNELVER)" = "5.4.164" ]] || [[ "$(KERNELVER)" = "5.4.213" ]]; then \
		$(call makemod, $(LINUX_KERNEL_PATH)/drivers/char/diag, diagchar.ko);\
		fi; \
	else $(call makemod, $(LINUX_KERNEL_PATH)/drivers/char/diag, diagchar.ko);\
	fi

ifeq ($(strip ${CONFIG_USB_SUPPORT}),y)
usb_f_diag_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget/function/, usb_f_diag.ko);
	@if [ "$*" = "install" ]; then \
		if [ -d $(SDK_FTM_INSTALL) ]; then \
		cp $(LINUX_KERNEL_PATH)/drivers/usb/gadget/function/usb_f_diag.ko $(SDK_FTM_INSTALL); \
		fi;\
		if [[ "$(KERNELVER)" = "5.4.164" ]] || [[ "$(KERNELVER)" = "5.4.213" ]]; then \
		$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget/function, *.ko);\
		fi; \
	else $(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget/function, *.ko);\
	fi		
else
usb_f_diag_%:
	@if [ "$*" = "install" ]; then \
		if [ -d $(SDK_FTM_INSTALL) ]; then \
		cp $(LINUX_KERNEL_PATH)/drivers/usb/gadget/function/usb_f_diag.ko $(SDK_FTM_INSTALL); \
		fi;\
		if [[ "$(KERNELVER)" = "5.4.164" ]] || [[ "$(KERNELVER)" = "5.4.213" ]]; then \
		$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget/function, *.ko);\
		fi; \
	else $(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget/function, *.ko);\
	fi
endif
	
ifeq ($(strip ${CONFIG_USB_SUPPORT}),y)	
udc-core_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget/udc, udc-core.ko);
	@if [ "$*" = "install" ]; then \
		if [ -d $(SDK_FTM_INSTALL) ]; then \
		cp $(LINUX_KERNEL_PATH)/drivers/usb/gadget/udc/udc-core.ko $(SDK_FTM_INSTALL); \
		fi;\
		if [[ "$(KERNELVER)" = "5.4.164" ]] || [[ "$(KERNELVER)" = "5.4.213" ]]; then \
		$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget/udc, udc-core.ko);\
		fi; \
	else $(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget/udc, udc-core.ko);\
	fi		
else
udc-core_%:
	@if [ "$*" = "install" ]; then \
		if [ -d $(SDK_FTM_INSTALL) ]; then \
		cp $(LINUX_KERNEL_PATH)/drivers/usb/gadget/udc/udc-core.ko $(SDK_FTM_INSTALL); \
		fi;\
		if [[ "$(KERNELVER)" = "5.4.164" ]] || [[ "$(KERNELVER)" = "5.4.213" ]]; then \
		$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget/udc, udc-core.ko);\
		fi; \
	else $(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget/udc, udc-core.ko);\
	fi
endif

ifeq ($(strip ${CONFIG_USB_SUPPORT}),y)	
libcomposite_%:
	$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget, libcomposite.ko);
	@if [ "$*" = "install" ]; then \
		if [ -d $(SDK_FTM_INSTALL) ]; then \
		cp $(LINUX_KERNEL_PATH)/drivers/usb/gadget/libcomposite.ko $(SDK_FTM_INSTALL); \
		fi;\
		if [[ "$(KERNELVER)" = "5.4.164" ]] || [[ "$(KERNELVER)" = "5.4.213" ]]; then \
		$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget, *.ko);\
		fi; \
	else $(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget, *.ko);\
	fi		
else
libcomposite_%:
	@if [ "$*" = "install" ]; then \
		if [ -d $(SDK_FTM_INSTALL) ]; then \
		cp $(LINUX_KERNEL_PATH)/drivers/usb/gadget/libcomposite.ko $(SDK_FTM_INSTALL); \
		fi;\
		if [[ "$(KERNELVER)" = "5.4.164" ]] || [[ "$(KERNELVER)" = "5.4.213" ]]; then \
		$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget, *.ko);\
		fi; \
	else $(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/gadget, *.ko);\
	fi
endif
	
usb_common_%:
	@if [ "$*" = "install" ]; then \
		if [ -d $(SDK_FTM_INSTALL) ]; then \
		cp $(LINUX_KERNEL_PATH)/drivers/usb/common/usb-common.ko $(SDK_FTM_INSTALL); \
		fi;\
		if [[ "$(KERNELVER)" = "5.4.164" ]] || [[ "$(KERNELVER)" = "5.4.213" ]]; then \
		$(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/common, *.ko);\
		fi; \
	else $(call makemod, $(LINUX_KERNEL_PATH)/drivers/usb/common, *.ko);\
	fi

crc-ccitt_%:
	@if [ "$*" = "install" ]; then \
		if [ -d $(SDK_FTM_INSTALL) ]; then \
		cp $(LINUX_KERNEL_PATH)/lib/crc-ccitt.ko $(SDK_FTM_INSTALL); \
		fi;\
		if [[ "$(KERNELVER)" = "5.4.164" ]] || [[ "$(KERNELVER)" = "5.4.213" ]]; then \
		$(call makemod, $(LINUX_KERNEL_PATH)/lib/, crc-ccitt.ko);\
		fi; \
	else $(call makemod, $(LINUX_KERNEL_PATH)/lib/, crc-ccitt.ko);\
	fi
	
############################ linux kernel rules ###################################
sdk_sync:
	@if test -e $(TOPDIR)/sdk/qca/.git; then \
		echo "git pull qca sdk..."; \
		cd $(TOPDIR)/sdk/qca && git checkout $(SDK_PACKAGE) && git pull; \
	else \
		echo "git checkout qca sdk..."; \
		cd $(TOPDIR) && git submodule init ./sdk/qca && git submodule update ./sdk/qca; \
		cd $(TOPDIR)/sdk/qca/ && git checkout $(SDK_PACKAGE) && git pull; \
	fi;

sdk_prep: sdk_sync
	@cd $(TOPDIR)/sdk/qca;
	@if test -L $(LINUX_KERNEL_PATH); then \
		rm -rf $(LINUX_KERNEL_PATH); \
	fi;
	@ln -s $(TOPDIR)/sdk/qca/$(KERNEL) $(LINUX_KERNEL_PATH);
	@[ -d $(LINUX_KERNEL_PATH)/user_headers ] || \
		make -C $(LINUX_KERNEL_PATH) HOSTCFLAGS="-O2 -Wall -Wmissing-prototypes -Wstrict-prototypes" CROSS_COMPILE=$(TOOLPREFIX) ARCH=$(KERNELARCH) KBUILD_HAVE_NLS=no CONFIG_SHELL="bash" V='' CC=$(TOOLPREFIX)gcc INSTALL_HDR_PATH=$(LINUX_KERNEL_PATH)/user_headers headers_install; \
		mkdir -p $(QCA_SDK_ROOT_PATH);
	@if test -L $(QCA_SDK_ROOT_PATH)/ethernet_driver; then \
		rm -rf $(QCA_SDK_ROOT_PATH)/ethernet_driver; \
	fi;
	@rm -rf $(QCA_SDK_ROOT_PATH)/* && ln -s $(TOPDIR)/sdk/qca/* $(QCA_SDK_ROOT_PATH)/;
	@rm -rf $(WLAN_TOPDIR) && ln -s $(TOPDIR)/sdk/qca/qca-wifi $(WLAN_TOPDIR);
	@mkdir -p $(SDK_INSTALL)/pkginfo;
	@mkdir -p $(SDK_INSTALL)/usr/include;
	@cp -f $(CONFIGDIR)/*.dts $(LINUX_KERNEL_PATH)/arch/arm64/boot/dts/qcom/;

ifeq ($(INCLUDE_CPU_QCA_IPQ807X), y)
	ln -f -s arch/nss_ipq807x_64.h $(QCA_SDK_ROOT_PATH)/qca-nss-drv/exports/nss_arch.h
ifneq ($(findstring $(CONFIG_TP_QCA_SDK_VERSION_CODE),$(TP_SPF_11_4_0)), )
	ln -f -s ../hal/arch/ipq807x/nss_ipq807x.h $(QCA_SDK_ROOT_PATH)/ethernet_driver/qca-nss-dp/exports/nss_dp_arch.h
endif
else ifeq ($(INCLUDE_CPU_QCA_IPQ60XX), y)
	ln -f -s arch/nss_ipq60xx_64.h $(QCA_SDK_ROOT_PATH)/qca-nss-drv/exports/nss_arch.h
else ifeq ($(INCLUDE_CPU_QCA_IPQ957X), y)
	ln -sf ../hal/soc_ops/ipq95xx/nss_ipq95xx.h $(QCA_SDK_ROOT_PATH)/ethernet_driver/qca-nss-dp/exports/nss_dp_arch.h
else ifeq ($(INCLUDE_CPU_QCA_IPQ53XX), y)
	ln -sf ../hal/soc_ops/ipq53xx/nss_ipq53xx.h $(QCA_SDK_ROOT_PATH)/ethernet_driver/qca-nss-dp/exports/nss_dp_arch.h
endif

sdk_clean:
	@cd $(QCA_SDK_ROOT_PATH)/ethernet_driver/qca-ssdk && $(ARCHMAKE) clean
	#@cd $(QCA_SDK_ROOT_PATH)/ethernet_driver/qca-nss-dp && $(ARCHMAKE) clean
ifneq ($(INCLUDE_CPU_QCA_IPQ957X)$(INCLUDE_CPU_QCA_IPQ53XX), )
	#@cd $(QCA_SDK_ROOT_PATH)/ethernet_driver/qca-nss-ppe && $(ARCHMAKE) clean
	#@cd $(QCA_SDK_ROOT_PATH)/ethernet_driver/qca-nss-ppe-rule && $(ARCHMAKE) clean
	#@cd $(QCA_SDK_ROOT_PATH)/ethernet_driver/qca-nss-ppe-vp && $(ARCHMAKE) clean
	#@cd $(QCA_SDK_ROOT_PATH)/ethernet_driver/qca-nss-ppe-ds && $(ARCHMAKE) clean
endif


ifeq ($(INCLUDE_CPU_QCA_IPQ957X), y)
SSDK_CHIP_TYPE:=APPE
else ifeq ($(INCLUDE_CPU_QCA_IPQ53XX), y)
SSDK_CHIP_TYPE:=MPPE
endif

sdk_build:
ifneq ($(INCLUDE_CPU_QCA_IPQ957X)$(INCLUDE_CPU_QCA_IPQ53XX), )
	#qca-ssdk
	@echo "building qca-ssdk..."
	cd $(QCA_SDK_ETH_PATH)/qca-ssdk && \
		$(ARCHMAKE) TOOL_PATH=$(TOOLCHAIN_INSTALL)/$(TOOLCHAIN_PACKAGE)/bin \
		SYS_PATH=$(LINUX_KERNEL_PATH) KVER=$(KERNELVER) ARCH=$(KERNELARCH) TARGET_SUFFIX="musl" GCC_VERSION=70500 Soc=$(SOC) CHIP_TYPE=$(SSDK_CHIP_TYPE)
	@cd $(QCA_SDK_ETH_PATH)/qca-ssdk && grep -F qca-ssdk Module.symvers > Module.symvers.tmp && sort -u Module.symvers.tmp && cp Module.symvers.tmp $(SDK_INSTALL)/pkginfo/qca-ssdk.symvers;
	#nat46
	@echo "building nat46..."
	@cd $(QCA_SDK_ETH_PATH)/nat46 && \
		$(MODULESMAKE) M=$(QCA_SDK_ETH_PATH)/nat46/nat46/modules MODFLAGS="-DMODULE -mlong-calls" EXTRA_CFLAGS="-DNAT46_VERSION=\\\"1182f30785e4274913f01a8c3d7e1b5437ae3819\\\"" SoC=$(SOC) modules
	@cd $(QCA_SDK_ETH_PATH)/nat46/nat46/modules && grep -F nat46 Module.symvers > Module.symvers.tmp && sort -u Module.symvers.tmp && cp Module.symvers.tmp $(SDK_INSTALL)/pkginfo/nat46.symvers;
	@cd $(QCA_SDK_ETH_PATH)/nat46/nat46/modules && cp *.h ../

	#qca-nss-ppe
	@echo "building qca-nss-ppe..."
	@cd $(QCA_SDK_ETH_PATH)/ && cp -rf qca-nss-ppe/ qca-nss-ppe-g/ && cd qca-nss-ppe-g/ && \
		cat $(SDK_INSTALL)/pkginfo/*.symvers 2>/dev/null > Module.symvers && \
		$(MODULESMAKE) $(QCA_NSS_PPE_EXTRA_OPT) M=$(QCA_SDK_ETH_PATH)/qca-nss-ppe-g/ EXTRA_CFLAGS="-I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/ -I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/fal -I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/init -I$(QCA_SDK_ETH_PATH)/nat46/" SoC=$(SOC) modules
	@cd $(QCA_SDK_ETH_PATH)/qca-nss-ppe-g/ && grep -F qca-nss-ppe-g Module.symvers > Module.symvers.tmp && sort -u Module.symvers.tmp && cp Module.symvers.tmp $(SDK_INSTALL)/pkginfo/qca-nss-ppe-g.symvers;

	#qca-nss-dp
	@echo "building qca-nss-dp..."
	@cd $(QCA_SDK_ETH_PATH)/qca-nss-dp && \
		cat $(SDK_INSTALL)/pkginfo/*.symvers 2>/dev/null > Module.symvers && \
		$(MODULESMAKE) $(QCA_NSS_DP_EXTRA_OPT) M=$(QCA_SDK_ETH_PATH)/qca-nss-dp EXTRA_CFLAGS="-I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/ -I$(QCA_SDK_ETH_PATH)/qca-nss-ppe/drv/exports/" SoC=$(SOC) modules
	@cd $(QCA_SDK_ETH_PATH)/qca-nss-dp && grep -F qca-nss-dp Module.symvers > Module.symvers.tmp && sort -u Module.symvers.tmp && cp Module.symvers.tmp $(SDK_INSTALL)/pkginfo/qca-nss-dp.symvers;
	#qca-nss-ppe-vp
	@echo "building qca-nss-ppe-vp"
	cd $(QCA_SDK_ETH_PATH)/ && cp -rf qca-nss-ppe/ qca-nss-ppe-vp/ && cd qca-nss-ppe-vp/ && \
		cat $(SDK_INSTALL)/pkginfo/*.symvers 2>/dev/null > Module.symvers && \
		$(MODULESMAKE) ppe-vp=y M=$(QCA_SDK_ETH_PATH)/qca-nss-ppe-vp EXTRA_CFLAGS="-I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/ -I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/fal -I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/init -I$(QCA_SDK_ETH_PATH)/nat46/ -I$(QCA_SDK_ETH_PATH)/qca-nss-dp/exports" SoC=$(SOC) modules
	@cd $(QCA_SDK_ETH_PATH)/qca-nss-ppe-vp && grep -F qca-nss-ppe-vp Module.symvers > Module.symvers.tmp && sort -u Module.symvers.tmp && cp Module.symvers.tmp $(SDK_INSTALL)/pkginfo/qca-nss-ppe-vp.symvers;
	@cd $(QCA_SDK_ETH_PATH)/qca-nss-ppe-vp && cp drv/exports/* $(SDK_INSTALL)/usr/include/

ifeq ($(CONFIG_TP_QCA_SDK_VERSION_CODE), $(TP_SPF_12_4_0))
	#qca-nss-ppe-client
	@echo "building qca-nss-ppe-client..."
	@cd $(QCA_SDK_ETH_PATH)/ && cp -rf qca-nss-ppe/ qca-nss-ppe-clients-g/ && cd qca-nss-ppe-clients-g/ && \
		cat $(SDK_INSTALL)/pkginfo/*.symvers 2>/dev/null > Module.symvers && \
		$(MODULESMAKE) bridge-mgr=y vlan-mgr=y pppoe-mgr=y lag-mgr=y M=$(QCA_SDK_ETH_PATH)/qca-nss-ppe-clients-g/ EXTRA_CFLAGS="-I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/ -I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/fal -I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/init -I$(QCA_SDK_ETH_PATH)/nat46/" SoC=$(SOC) modules
	@cd $(QCA_SDK_ETH_PATH)/qca-nss-ppe-clients-g/ && grep -F qca-nss-ppe-clients-g Module.symvers > Module.symvers.tmp && sort -u Module.symvers.tmp && cp Module.symvers.tmp $(SDK_INSTALL)/pkginfo/qca-nss-ppe-clients-g.symvers;
	@echo "qca-nss-ppe-clients build done..."
endif

	#qca-nss-ppe-rule
	@echo "building qca-nss-ppe-rule"
	cd $(QCA_SDK_ETH_PATH)/ && cp -rf qca-nss-ppe/ qca-nss-ppe-rule/ && cd qca-nss-ppe-rule/ && \
		cat $(SDK_INSTALL)/pkginfo/*.symvers 2>/dev/null > Module.symvers && \
		$(MODULESMAKE) ppe-rule=y PPE_RFS_ENABLED=y $(QCA_NSS_PPE_RULE_EXTRA_OPT) M=$(QCA_SDK_ETH_PATH)/qca-nss-ppe-rule EXTRA_CFLAGS="-I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/ -I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/fal -I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/init -I$(QCA_SDK_ETH_PATH)/nat46/ -I$(QCA_SDK_ETH_PATH)/qca-nss-dp/exports" SoC=$(SOC) modules
	@cd $(QCA_SDK_ETH_PATH)/qca-nss-ppe-rule && grep -F qca-nss-ppe-rule Module.symvers > Module.symvers.tmp && sort -u Module.symvers.tmp && cp Module.symvers.tmp $(SDK_INSTALL)/pkginfo/qca-nss-ppe-rule.symvers;
	#qca-nss-ppe-ds
	@echo "building qca-nss-ppe-ds"
	cd $(QCA_SDK_ETH_PATH)/ && cp -rf qca-nss-ppe/ qca-nss-ppe-ds/ && cd qca-nss-ppe-ds/ && \
		cat $(SDK_INSTALL)/pkginfo/*.symvers 2>/dev/null > Module.symvers && \
		$(MODULESMAKE) ppe-ds=y M=$(QCA_SDK_ETH_PATH)/qca-nss-ppe-ds EXTRA_CFLAGS="-I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/ -I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/fal -I$(QCA_SDK_ETH_PATH)/qca-ssdk/exports/include/init -I$(QCA_SDK_ETH_PATH)/nat46/ -I$(QCA_SDK_ETH_PATH)/qca-nss-dp/exports -I$(QCA_SDK_ETH_PATH)/qca-nss-ppe-g/drv/exports" SoC=$(SOC) modules
	@cd $(QCA_SDK_ETH_PATH)/qca-nss-ppe-ds && grep -F qca-nss-ppe-ds Module.symvers > Module.symvers.tmp && sort -u Module.symvers.tmp && cp Module.symvers.tmp $(SDK_INSTALL)/pkginfo/qca-nss-ppe-ds.symvers;
	@cd $(QCA_SDK_ETH_PATH)/qca-nss-ppe-ds/ && mkdir -p $(SDK_INSTALL)/usr/include/qca-nss-ppe && cp drv/ppe_ds/exports/* $(SDK_INSTALL)/usr/include/qca-nss-ppe

else
	@cd $(QCA_SDK_ROOT_PATH)/ethernet_driver/qca-ssdk && \
		$(ARCHMAKE) TOOL_PATH=$(TOOLCHAIN_INSTALL)/$(TOOLCHAIN_PACKAGE)/bin \
		SYS_PATH=$(LINUX_KERNEL_PATH) KVER=$(KERNELVER) ARCH=$(KERNELARCH) PTP_FEATURE=enable HK_CHIP=enable
	@cd $(QCA_SDK_ROOT_PATH)/ethernet_driver/qca-nss-dp && \
		$(MODULESMAKE) M=$(QCA_SDK_ROOT_PATH)/ethernet_driver/qca-nss-dp SoC=$(SOC) modules
	@cd $(QCA_SDK_ROOT_PATH)/qca-nss-drv && \
		$(MODULESMAKE) M=$(QCA_SDK_ROOT_PATH)/qca-nss-drv EXTRA_CFLAGS="-I$(QCA_SDK_ROOT_PATH)/ethernet_driver/qca-nss-dp/exports -I$(QCA_SDK_ROOT_PATH)/ethernet_driver/qca-ssdk/exports/include/" SoC=$(SOC) modules
	@cd $(QCA_SDK_ROOT_PATH)/qca-nss-drv && mkdir -p $(SDK_INSTALL)/usr/include/ && \
		cp -rf exports/* $(SDK_INSTALL)/usr/include/ && \
		if test -f nss_arch.h; then cp nss_arch.h $(SDK_INSTALL)/usr/include/; fi
endif

sdk_install:
ifneq ($(INCLUDE_CPU_QCA_IPQ957X)$(INCLUDE_CPU_QCA_IPQ53XX), )

ifeq ($(CONFIG_TP_QCA_SDK_VERSION_CODE), $(TP_SPF_12_4_0))
	#qca-nss-ppe-client
	@mkdir -p $(SMB_MOD_INSTALL) && cd $(QCA_SDK_ROOT_PATH) && \
		cp ethernet_driver/qca-ssdk/*.ko \
		ethernet_driver/qca-nss-ppe-g/drv/ppe_drv/*.ko  \
		ethernet_driver/qca-nss-ppe-clients-g/clients/vlan/*.ko \
		ethernet_driver/qca-nss-ppe-clients-g/clients/lag/*.ko \
		ethernet_driver/qca-nss-ppe-clients-g/clients/pppoe/*.ko \
		ethernet_driver/qca-nss-ppe-clients-g/clients/bridge/*.ko \
		ethernet_driver/qca-nss-ppe-rule/drv/ppe_rule/*.ko \
		ethernet_driver/qca-nss-ppe-vp/drv/ppe_vp/*.ko \
		ethernet_driver/qca-nss-ppe-ds/drv/ppe_ds/*.ko \
		ethernet_driver/qca-nss-dp/*.ko \
		ethernet_driver/nat46/nat46/modules/*.ko \
		$(SMB_MOD_INSTALL) 
else
	#qca-nss-ppe-client
	@mkdir -p $(SMB_MOD_INSTALL) && cd $(QCA_SDK_ROOT_PATH) && \
		cp ethernet_driver/qca-ssdk/*.ko \
		ethernet_driver/qca-nss-ppe-g/drv/ppe_drv/*.ko  \
		ethernet_driver/qca-nss-ppe-g/clients/vlan/*.ko \
		ethernet_driver/qca-nss-ppe-g/clients/lag/*.ko \
		ethernet_driver/qca-nss-ppe-g/clients/pppoe/*.ko \
		ethernet_driver/qca-nss-ppe-g/clients/bridge/*.ko \
		ethernet_driver/qca-nss-ppe-rule/drv/ppe_rule/*.ko \
		ethernet_driver/qca-nss-ppe-vp/drv/ppe_vp/*.ko \
		ethernet_driver/qca-nss-ppe-ds/drv/ppe_ds/*.ko \
		ethernet_driver/qca-nss-dp/*.ko \
		ethernet_driver/nat46/nat46/modules/*.ko \
		$(SMB_MOD_INSTALL) 
endif

else
	@mkdir -p $(SMB_MOD_INSTALL) && cd $(QCA_SDK_ROOT_PATH) && \
		cp ethernet_driver/qca-ssdk/*.ko $(SMB_MOD_INSTALL) && cp ethernet_driver/qca-nss-dp/*.ko $(SMB_MOD_INSTALL) && \
		cp qca-nss-drv/*.ko $(SMB_MOD_INSTALL)
endif

kernel_config:
	@cp -f $(KERNEL_CONFIG_FILE) $(KERNEL_CONFIG_TEMP)
	
	@if test "$(CONFIG_TP_IGMP_SNOOPING)" = "y"; then \
		sed -i 's/# CONFIG_TP_MULTICAST is not set/CONFIG_TP_MULTICAST=y/' $(KERNEL_CONFIG_TEMP);\
	else \
		sed -i 's/CONFIG_TP_MULTICAST=y/# CONFIG_TP_MULTICAST is not set/' $(KERNEL_CONFIG_TEMP);\
	fi;

	@if test "$(TP_MEM_ROOTFS)" = "n" ; then \
		sed -i 's/CONFIG_TPLINK_MEM_ROOTFS=y/# CONFIG_TPLINK_MEM_ROOTFS is not set/' $(KERNEL_CONFIG_TEMP); \
	else \
		sed -i 's/# CONFIG_TPLINK_MEM_ROOTFS is not set/CONFIG_TPLINK_MEM_ROOTFS=y/' $(KERNEL_CONFIG_TEMP); \
	fi;
	
	@mv -f $(KERNEL_CONFIG_TEMP) $(LINUX_KERNEL_PATH)/arch/$(KERNELARCH)/configs/temp_defconfig

	@cd $(LINUX_KERNEL_PATH) && if diff -I'^#.*' .config $(KERNEL_CONFIG_FILE); then \
		echo "kernel config file has no changes."; \
	else \
		$(ARCHMAKE) temp_defconfig; \
	fi;

kernel_menuconfig:
	@cd $(LINUX_KERNEL_PATH) && cp -f $(KERNEL_CONFIG_FILE) .config && $(ARCHMAKE) menuconfig && cp -f .config $(KERNEL_CONFIG_FILE)

kernel_info:
	@[ -d $(IMAGE_INSTALL) ] || mkdir -p $(IMAGE_INSTALL)
	@cd $(LINUX_KERNEL_PATH) && \
		echo "****************************" 		> $(IMAGE_INSTALL)/vmlinux.info && \
		echo "Linux Entry: "${KERNLE_ENTRY}    >> $(IMAGE_INSTALL)/vmlinux.info && \
		echo "Load Address: 0x"${KERNLE_LDADR} >> $(IMAGE_INSTALL)/vmlinux.info && \
		echo "****************************"    >> $(IMAGE_INSTALL)/vmlinux.info;
	@cat $(IMAGE_INSTALL)/vmlinux.info

kernel_clean: 
	@cd $(LINUX_KERNEL_PATH) &&  $(ARCHMAKE) mrproper;

kernel_build: kernel_config
ifeq ($(KERNELVER), 4.4.60)
	cd $(LINUX_KERNEL_PATH) && $(ARCHMAKE) $(KERNELTARGET)
else ifeq ($(KERNELVER), 5.4.164)
	cp -f $(CONFIGDIR)/*.dts $(LINUX_KERNEL_PATH)/arch/arm64/boot/dts/qcom/
	cd $(LINUX_KERNEL_PATH) && $(ARCHMAKE) $(KERNELTARGET)
else ifeq ($(KERNELVER), 5.4.213)
	cp -f $(CONFIGDIR)/*.dts $(LINUX_KERNEL_PATH)/arch/arm64/boot/dts/qcom/
	cd $(LINUX_KERNEL_PATH) && $(ARCHMAKE) $(KERNELTARGET)
else
	cd $(LINUX_KERNEL_PATH) && $(ARCHMAKE) vmlinuz
	make kernel_info
endif

kernel_modules: kernel_config
	@cd $(LINUX_KERNEL_PATH) && $(ARCHMAKE) modules && find ./ -name "*.ko";

kernel_build2:
	@cd $(LINUX_KERNEL_PATH) && $(ARCHMAKE) vmlinux.bin && \
		printf '0:3c02800034421ffc8c59000003200008' | xxd -r >  vmlinux2.bin && \
		printf '0:000000000000000000000000'         | xxd -r >> vmlinux2.bin && \
		printf '0:%08x' ${KERNLE_ENTRY}             | xxd -r >> vmlinux2.bin && \
		cat arch/$(KERNELARCH)/boot/vmlinux.bin              >> vmlinux2.bin && \
		cp -f vmlinux2.bin $(IMAGE_INSTALL)
	@echo "\nBuild vmlinux2.bin Done.\n"

DTBPATH=$(LINUX_KERNEL_PATH)/arch/$(KERNELARCH)/boot/dts/qcom
ifneq ($(DTB_NAME), )
	DTBNAME=$(DTB_NAME)
else
	DTBNAME=$(DTBPREFIX)
endif

kernel_install:
	@if [[ "$(KERNELVER)" = "4.4.60" || "$(KERNELVER)" = "5.4.164" || "$(KERNELVER)" = "5.4.213" ]] ; then \
		cd $(IMAGE_INSTALL) && \
		cp $(LINUX_KERNEL_PATH)/arch/$(KERNELARCH)/boot/Image .  && \
		gzip -9 -c Image > ./Image.gz  && \
		$(PRODUCT_SCRIPT_PATH)/mkits.sh -D $(DTBNAME) -o ./fit-$(DTBNAME).its -k ./Image.gz $(foreach dtb, $(DTBPREFIX), -d $(DTBPATH)/$(dtb).dtb) -C gzip -a $(KERNEL_LOAD_ADDR) -e $(KERNEL_ENTRY_ADDR) -A $(KERNELARCH) -v $(KERNELVER) && \
		PATH=$(LINUX_KERNEL_PATH)/scripts/dtc:$(PATH)  && \
		$(UTIL_PATH)/mkimage -f ./fit-$(DTBNAME).its ./$(BOARD_TYPE)-fit-uImage.itb  && \
		rm -rf Image  && \
		rm -rf Image.gz  && \
		rm -rf fit-$(DTBNAME).its; \
	else \
		cd $(LINUX_KERNEL_PATH) && cp vmlinuz vmlinux $(IMAGE_INSTALL); \
	fi;

############################# image rules #####################################

uimage:kernel_config
	@cd $(LINUX_KERNEL_PATH) && $(ARCHMAKE) vmlinux.bin && \
		cp -f arch/$(KERNELARCH)/boot/vmlinux.bin $(LINUX_KERNEL_PATH);
	@mkuImage.sh $(UTIL_PATH) $(LINUX_KERNEL_PATH) "" $(UIMAGE_COMPRESS);

lzma_uimage:
	@make uimage UIMAGE_COMPRESS=lzma;

tpimage:
	@test -n  "$(MODEL_NAME)" -a -n "$(BOOT_NAME)" || \
		(echo "MODEL_NAME or BOOT_NAME is undefined!" && exit 1)
	@test -f $(IMAGE_INSTALL)/squashfs || (echo "squashfs is not exits" && exit 1)
	@cd $(IMAGE_INSTALL) && case "$(KERNEL_NAME)" in \
		*.trx) trx -o $(KERNEL_NAME) vmlinuz;; esac;
	@cd $(IMAGE_INSTALL) && make_flash \
		-k `test -n "$(KERNEL_NAME)" && echo $(KERNEL_NAME) || echo vmlinuz` \
		-b $(BOOT_NAME) -f squashfs \
		-o $(MODEL_NAME) -c model.conf -t buildtime.h

############################### rootfs #######################################
ifdef GPL_BUILD
export BOARD_ROOTFS = fscopy uclibc emptys strip mkdev $(ROOTFS_ADDITIONAL)
else
export BOARD_ROOTFS = fscopy uclibc wireless webpages emptys strip mkdev $(ROOTFS_ADDITIONAL)
endif

ifdef CONFIG_NM_NAND_TYPE
BOARD_ROOTFS += ubi
endif

############################### rootfs rules #######################################

rootfs_undev:
	@rm -rf $(ROOTFS_BUILD_PATH)/dev/

rootfs_mkdev: rootfs_undev
	@$(PRODUCT_SCRIPT_PATH)/mkdev $(ROOTFS_BUILD_PATH)

rootfs_uclibc:
	@if [[ "$(KERNELVER)" = "4.4.60" ]] ; then \
		cp -f $(TOOLCHAIN_INSTALL)/$(gcc_package)/lib/libc.so $(ROOTFS_BUILD_PATH)/lib/libc.so; \
		cp -f $(TOOLCHAIN_INSTALL)/$(gcc_package)/lib/librpc.so $(ROOTFS_BUILD_PATH)/lib/librpc.so; \
		cp -f $(TOOLCHAIN_INSTALL)/$(gcc_package)/lib/libgcc_s.so.1 $(ROOTFS_BUILD_PATH)/lib/libgcc_s.so.1; \
		cp -f $(TOOLCHAIN_INSTALL)/$(gcc_package)/lib/libssp.so.0.0.0 $(ROOTFS_BUILD_PATH)/lib/libssp.so.0; \
	elif [[ "$(KERNELVER)" = "5.4.164" ]] || [[ "$(KERNELVER)" = "5.4.213" ]] ; then \
		echo "============test 5.4.213============"; \
		cp -f $(TOOLCHAIN_INSTALL)/$(gcc_package)/lib/libc.so $(ROOTFS_BUILD_PATH)/lib/libc.so; \
		cp -f $(TOOLCHAIN_INSTALL)/$(gcc_package)/lib/librpc.so $(ROOTFS_BUILD_PATH)/lib/librpc.so; \
		cp -f $(TOOLCHAIN_INSTALL)/$(gcc_package)/lib/libgcc_s.so.1 $(ROOTFS_BUILD_PATH)/lib/libgcc_s.so.1; \
	else \
		$(UTIL_PATH)/cplibs $(TOOLCHAIN_INSTALL)/$(gcc_package) $(ROOTFS_BUILD_PATH) $(NEED_LIBCPP); \
	fi;

rootfs_emptys:
	@cd $(ROOTFS_BUILD_PATH) && \
		rm -rf root mnt proc tmp var && \
		mkdir -p root mnt proc tmp var
ifneq ($(INCLUDE_CPU_QCA_IPQ807X)$(INCLUDE_CPU_QCA_IPQ957X)$(INCLUDE_CPU_QCA_IPQ53XX), )
	@cd $(ROOTFS_BUILD_PATH) && \
		rm -rf data && mkdir -p data
endif

export STRIP_FILES = lib/lib*.so lib/lib*.so.* lib/libexec/xtables/lib*.so \
				sbin/* bin/* usr/sbin/* usr/local/sbin/* usr/bin/*
export CHMOD_FILES = shadow passwd app_features.json default_config gpio.conf led.conf nrd.conf oem.json radio_config.json

STRIP_EVERY_THING?=y
rootfs_strip:
	@find $(ROOTFS_BUILD_PATH) -name ".svn" | xargs rm -rf
	rm -rf lib/*.a
	
	@if test "$(STRIP_EVERY_THING)" = "y"; then \
		echo "strip start"; \
		cd $(ROOTFS_BUILD_PATH) && \
		for f in $(STRIP_FILES); \
		do if test -f $$f; then \
		result=`file $$f | grep ELF`; \
		if [[ "$$result" != "" ]]; then ${RSTRIP} $$f; fi; \
		fi; \
		done \
	else \
		echo "strip skips";\
	fi;
	
	@chmod 777 $(ROOTFS_BUILD_PATH) -R
	cd $(ROOTFS_BUILD_PATH)/etc && for f in $(CHMOD_FILES); do if test -f $$f; then chmod 600 $$f; fi; done 
	@if [[ "$(KERNELVER)" = "4.4.60" || "$(KERNELVER)" = "5.4.164" || "$(KERNELVER)" = "5.4.213" ]] ; then \
		echo "strip ko start"; \
		$(RSTRIP) $(ROOTFS_BUILD_PATH)/lib/modules/$(KERNELVER)/ ;\
	fi;

rootfs_fscopy:
	@[ -d $(ROOTFS_BUILD_PATH) ] || mkdir -p $(ROOTFS_BUILD_PATH)
	@cp -Rf $(INSTALL_ROOT)/* $(ROOTFS_BUILD_PATH)
	@rm -rf $(ROOTFS_BUILD_PATH)/wireless
	@if test "$(INC_NART_OUT)" = "y"; then \
		cp -f $(IMAGE_INSTALL)/nart.out $(ROOTFS_BUILD_PATH)/sbin;\
	else \
		rm -rf $(ROOTFS_BUILD_PATH)/sbin/nart.out;\
	fi;
	
	@if [[ "$(TDB_ENABLE)" = "y" && "$(KERNELARCH)" = "mips" ]] ; then \
		touch $(ROOTFS_BUILD_PATH)/etc/tdbEnable; \
	else \
		rm -rf $(ROOTFS_BUILD_PATH)/etc/tdbEnable; \
		echo "tdb disable";\
	fi;

	@if test "$(CONFIG_EAP_MONITOR_SUPPORT)" = "y"; then \
		touch $(ROOTFS_BUILD_PATH)/etc/eapMonitorEnable; \
	else \
		rm -rf $(ROOTFS_BUILD_PATH)/etc/eapMonitorEnable; \
		echo "eap-monitor disable";\
	fi;
	@echo make $@ done.

rootfs_model:
	@cd $(IMAGE_INSTALL) && echo -n "BUILD_TIME = " > buildtime.h && \
		expr `date +%H` \* 3600 + `date +%M` \* 60 + `date +%S` >> buildtime.h && \
		echo -n "BUILD_DATE = " >> buildtime.h && date +%D >> buildtime.h;
	@test -n  "$(WEBV4_PATH)" -a -n "$(MODEL_NAME)" || \
		(echo "WEBV4_PATH or MODEL_NAME is undefined!" && exit 1)
	@cd $(WEBV4_PATH)/models && \
		cp $(MODEL_NAME).conf model.conf && \
		echo "" >> model.conf && cat version.conf >> model.conf && \
		echo "" >> model.conf && cat $(IMAGE_INSTALL)/buildtime.h >> model.conf && \
		mv model.conf $(IMAGE_INSTALL)
	@cd $(ROOTFS_BUILD_PATH) && mkdir -p web/oem && \
		$(UTIL_PATH)/md5_des $(IMAGE_INSTALL)/model.conf web/oem/model.conf enc
	@echo make $@ done.

ifeq ($(strip ${CONFIG_NAND_MULTI_BOARD}), y)
COMMON=common/
else
COMMON=
endif
rootfs_ubi:
ifndef GPL_BUILD
	mkdir -p $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)
	rm $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)/* -rf
	mkdir -p $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)/$(USER_CONF_DIR)
	mkdir -p $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)/$(MANU_CONF_DIR)

	mkdir -p $(ROOTFS_BUILD_PATH)/tp_data
	mkdir -p $(ROOTFS_BUILD_PATH)/tp_data/$(MANU_CONF_DIR)
	mkdir -p $(ROOTFS_BUILD_PATH)/tp_data/$(MANU_BACKUP_DIR)
	mkdir -p $(ROOTFS_BUILD_PATH)/tp_data/$(USER_CONF_DIR)

	$(GENSOFTVER) -d -o $(IMAGE_INSTALL)/soft-version.bin -a $(IMAGE_INSTALL)/additional-hardversion.bin -c $(MKIMG_PATH)/$(COMMON)build.ini
ifeq ($(strip ${CONFIG_SUPPORT_FACTORY_BACKUP}),1)
	cp -Rf $(MKIMAGE_PAH) $(IMAGE_INSTALL)/ ;
	cd $(IMAGE_BUILD_DIR) && \
	[ -d $(RUNTIME_BACKUP_DIR) ] || mkdir -p $(RUNTIME_BACKUP_DIR)  && \
	[ -d $(FACTORY_DATA_DIR) ]   || mkdir -p $(FACTORY_DATA_DIR)    && \
	[ -d $(FACTORY_BACKUP_DIR) ] || mkdir -p $(FACTORY_BACKUP_DIR)  && \
	touch $(RUNTIME_BACKUP_DIR)/user-config  && \
	cp $(IMAGE_INSTALL)/additional-hardversion.bin $(FACTORY_DATA_DIR)/add-hardver && \
	cp $(IMAGE_INSTALL)/additional-hardversion.bin $(FACTORY_BACKUP_DIR)/add-hardver && \
	$(UTIL_PATH)/mkfs.ubifs -F -m 2048 -e 124KiB -c 60 -r $(RUNTIME_BACKUP_DIR) -o runtime_back.ubifs && \
	$(UTIL_PATH)/mkfs.ubifs -F -m 2048 -e 124KiB -c 60 -r $(FACTORY_DATA_DIR) -o factory_data.ubifs && \
	$(UTIL_PATH)/mkfs.ubifs -F -m 2048 -e 124KiB -c 60 -r $(FACTORY_BACKUP_DIR) -o backup_data.ubifs && \
	$(UTIL_PATH)/ubinize -m 2048 -p 128KiB -o runtime_back.ubi $(COMMON)runtime_back.cfg && \
	$(UTIL_PATH)/ubinize -m 2048 -p 128KiB -o factory_data.ubi $(COMMON)factory_data.cfg && \
	$(UTIL_PATH)/ubinize -m 2048 -p 128KiB -o backup_data.ubi $(COMMON)backup_data.cfg


	cp $(IMAGE_BUILD_DIR)/runtime_back.ubi $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)/runtime_back.ubi
	cp $(IMAGE_BUILD_DIR)/factory_data.ubi $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)/factory_data.ubi
	cp $(IMAGE_BUILD_DIR)/backup_data.ubi $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)/backup_data.ubi
endif
ifeq ($(strip ${CONFIG_NAND_MULTI_BOARD}), y)
	$(foreach board, $(CONFIG_MULTI_BOARD_LIST), cp $(MKIMG_PATH)/$(board)/productinfo.bin $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)/product-info.$(board);)
	cp $(MKIMG_PATH)/$(COMMON)productinfo.bin.default $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)/product-info
else
	cp $(MKIMG_PATH)/productinfo.bin $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)/product-info
	cp $(MKIMG_PATH)/default-config.bin $(ROOTFS_BUILD_PATH)/etc/factory_config.bin
endif
	cp $(MKIMG_PATH)/$(COMMON)supportlist.bin $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)/support-list
	cp $(MKIMG_PATH)/$(COMMON)partition.conf $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)/partition-table
	cp $(IMAGE_INSTALL)/soft-version.bin $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)/soft-version
	cp $(MKIMG_PATH)/$(COMMON)fw_env.config $(ROOTFS_BUILD_PATH)/etc/fw_env.config
ifeq ($(strip ${CONFIG_BLE_SUPPORT}),y)
	cp $(MKIMG_PATH)/$(COMMON)ble_* $(ROOTFS_BUILD_PATH)/etc/
endif
	for f in $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)/*; do if test -f $$f; then chmod 774 $$f; fi; done;
else
	cp -Rf $(MKIMAGE_PAH) $(IMAGE_INSTALL)/ ;
endif

rootfs_sync_base:
ifndef GPL_BUILD
	cp -rf $(TOPDIR)/images/base/$(PID)/rootfs_base/* $(TOPDIR)/images/$(PID)/$(BOARD_TYPE)/rootfs/
	cp -rf $(TOPDIR)/images/base/$(PID)/$(BOARD_TYPE)  $(TOPDIR)/images/$(PID)/
	@echo $@ done.
endif

rootfs_clean: rootfs_undev
	@rm -rf $(ROOTFS_BUILD_PATH)
	@echo make $@ done.

ifndef GPL_BUILD
rootfs_build:  rootfs_sync_base webpages_sync
	@echo make $@ done.
else
rootfs_build:  rootfs_sync_base
	@echo make $@ done.
endif

rootfs_fakeroot: $(addprefix rootfs_,  $(filter-out $(ROOTFS_EXCLUDE), $(BOARD_ROOTFS)))
	@if [[ "$(KERNELVER)" = "4.4.60" || "$(KERNELVER)" = "5.4.164" || "$(KERNELVER)" = "5.4.213" ]] ; then \
		$(UTIL_PATH)/$(MKSQUASHFS) $(ROOTFS_BUILD_PATH) $(IMAGE_INSTALL)/squashfs \
			-nopad -noappend -root-owned $(comp_option) -Xpreset 9 -Xe -Xlc 0 -Xlp 2 -Xpb 2 -b 256k -p '/dev d 755 0 0' -p '/dev/console c 600 0 0 5 1' -processors 1; \
		$(UTIL_PATH)/padjffs2 $(IMAGE_INSTALL)/squashfs 4 8 16 64 128 256; \
		dd if=$(IMAGE_INSTALL)/squashfs of=$(IMAGE_INSTALL)/$(BOARD_TYPE)-squashfs-root.img bs=2k conv=sync; \
		rm $(IMAGE_INSTALL)/squashfs; \
	else \
		$(UTIL_PATH)/$(MKSQUASHFS) \
			$(ROOTFS_BUILD_PATH) $(IMAGE_INSTALL)/squashfs \
			-noappend -always-use-fragments -all-root $(comp_option) -processors 2; \
	fi;

rootfs_install:
	@$(UTIL_PATH)/fakeroot make rootfs_fakeroot
	@echo make $@ done.

image_clean:
	@echo "make $@."
	rm -rf $(IMAGE_INSTALL)/mkimage $(IMAGE_INSTALL)/*.bin;

image_build: nand_flash_maker rootfs_build
ifdef CONFIG_NM_NAND_TYPE
	@echo "make $@.";

	@if [ ! -f "$(INSTALL_ROOT)/etc/default-caldata.bin" ] ; then \
		echo "$(INSTALL_ROOT)/etc/default-caldata.bin is not exist!" ; \
		exit 1 ; \
	fi;

ifeq ($(strip ${CONFIG_NAND_MULTI_BOARD}), y)
	cp -Rf $(MKIMAGE_PAH) $(IMAGE_INSTALL)/ ;
	cd $(IMAGE_BUILD_DIR)  && \
		rm -rf ../*flash* ../*up.bin && \
		cd $(COMMON) && $(UTIL_PATH)/ubinize -m 2048 -p 128KiB -o root.ubi root.cfg && \
		cp $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)/soft-version soft-version.bin ;
	$(foreach board, $(CONFIG_MULTI_BOARD_LIST), \
		MODEL=`echo $(board) | cut -d'_' -f1`; \
		VER_X=`echo $(board) | cut -d'_' -f2`; \
		VER_Y=`echo $(board) | cut -d'_' -f3`; \
		if [ $${VER_Y} -gt 0 ] ; then UP_PREIFX=$${MODEL}v$${VER_X}.$${VER_Y}; \
		else UP_PREIFX=$${MODEL}v$${VER_X}; \
		fi; \
		cd $(IMAGE_BUILD_DIR)/$(board) && \
		ln -sf ../common . && \
		mkdir -p $(RUNTIME_DATA_DIR_NAME) && \
		mkdir -p $(FACTORY_DATA_DIR_NAME) && \
		mkdir -p $(FACTORY_BACKUP_DIR_NAME) && \
		cp default-config.bin $(RUNTIME_DATA_DIR_NAME)/user-config && \
		cp device-info.bin $(FACTORY_DATA_DIR_NAME)/device-info && \
		cp device-info.bin $(FACTORY_BACKUP_DIR_NAME)/device-info && \
		cp $(COMMON)additional-hardversion.bin $(FACTORY_DATA_DIR_NAME)/add-hardver && \
		cp $(COMMON)additional-hardversion.bin $(FACTORY_BACKUP_DIR_NAME)/add-hardver && \
		cp $(COMMON)default-mac $(FACTORY_DATA_DIR_NAME)/default-mac && \
		cp $(COMMON)default-mac $(FACTORY_BACKUP_DIR_NAME)/default-mac && \
		cp $(COMMON)device-id $(FACTORY_DATA_DIR_NAME)/device-id && \
		cp $(COMMON)device-id $(FACTORY_BACKUP_DIR_NAME)/device-id && \
		cp $(COMMON)region $(FACTORY_DATA_DIR_NAME)/region && \
		cp $(COMMON)region $(FACTORY_BACKUP_DIR_NAME)/region && \
		cp $(COMMON)default-caldata.bin $(FACTORY_DATA_DIR_NAME)/radio && \
		cp $(COMMON)default-caldata.bin $(FACTORY_BACKUP_DIR_NAME)/radio && \
		cp $(IMAGE_BUILD_DIR)/runtime_back.ubi ./ && \
		$(UTIL_PATH)/mkfs.ubifs -F -m 2048 -e 124KiB -c 88 -r $(RUNTIME_DATA_DIR_NAME) -o runtime_data.ubifs && \
		$(UTIL_PATH)/mkfs.ubifs -F -m 2048 -e 124KiB -c 60 -r $(FACTORY_DATA_DIR_NAME) -o factory_data.ubifs && \
		$(UTIL_PATH)/mkfs.ubifs -F -m 2048 -e 124KiB -c 60 -r $(FACTORY_BACKUP_DIR_NAME) -o backup_data.ubifs && \
		$(UTIL_PATH)/ubinize -m 2048 -p 128KiB -o runtime_data.ubi $(COMMON)runtime_data.cfg && \
		$(UTIL_PATH)/ubinize -m 2048 -p 128KiB -o factory_data.ubi $(COMMON)factory_data.cfg && \
		$(UTIL_PATH)/ubinize -m 2048 -p 128KiB -o backup_data.ubi $(COMMON)backup_data.cfg && \
		$(FLASHMAKER) -p $(COMMON)partition.conf -o $${UP_PREIFX} -i $(IMG_GEN) -d -b 128 && \
		find -name "*up.bin" -o -name "*flash*" |xargs mv -t ../../ ;)
else
	cp -Rf $(MKIMAGE_PAH) $(IMAGE_INSTALL)/ ;
	cd $(IMAGE_BUILD_DIR) && \
	rm -rf ../*flash* ../*up.bin && \
	[ -d $(RUNTIME_DATA_DIR) ] || mkdir -p $(RUNTIME_DATA_DIR) && \
	[ -d $(FACTORY_DATA_DIR) ] || mkdir -p $(FACTORY_DATA_DIR) && \
	[ -d $(FACTORY_BACKUP_DIR) ] || mkdir -p $(FACTORY_BACKUP_DIR) && \
	cp $(ROOTFS_BUILD_PATH)/$(FW_CONF_DIR)/soft-version $(IMAGE_BUILD_DIR)/soft-version.bin && \
	cp $(MKIMG_PATH)/default-config.bin $(RUNTIME_DATA_DIR)/user-config && \
	cp $(IMAGE_INSTALL)/additional-hardversion.bin $(FACTORY_DATA_DIR)/add-hardver && \
	cp $(IMAGE_INSTALL)/additional-hardversion.bin $(FACTORY_BACKUP_DIR)/add-hardver && \
	cp $(INSTALL_ROOT)/etc/default-caldata.bin $(FACTORY_DATA_DIR)/radio && \
	cp $(INSTALL_ROOT)/etc/default-caldata.bin $(FACTORY_BACKUP_DIR)/radio && \
	cp $(IMAGE_INSTALL)/mkimage/device-info.bin $(FACTORY_DATA_DIR)/device-info && \
	cp $(IMAGE_INSTALL)/mkimage/device-info.bin $(FACTORY_BACKUP_DIR)/device-info


ifdef CONFIG_CALDATA_TWO_FILES
	cd $(IMAGE_BUILD_DIR) && \
	cp $(INSTALL_ROOT)/etc/default-caldata.bin $(FACTORY_DATA_DIR)/radio1 && \
	cp $(INSTALL_ROOT)/etc/default-caldata.bin $(FACTORY_BACKUP_DIR)/radio1
endif

ifdef IMG_GEN_OOB128
	cp $(TOOLS_PATH)/img_gen_oob128 $(IMAGE_BUILD_DIR)
endif
	cd $(IMAGE_BUILD_DIR) && \
	$(UTIL_PATH)/mkfs.ubifs -F -m 2048 -e 124KiB -c 88 -r $(RUNTIME_DATA_DIR) -o runtime_data.ubifs && \
	$(UTIL_PATH)/mkfs.ubifs -F -m 2048 -e 124KiB -c 60 -r $(FACTORY_DATA_DIR) -o factory_data.ubifs && \
	$(UTIL_PATH)/mkfs.ubifs -F -m 2048 -e 124KiB -c 60 -r $(FACTORY_BACKUP_DIR) -o backup_data.ubifs && \
	$(UTIL_PATH)/ubinize -m 2048 -p 128KiB -o root.ubi root.cfg && \
	$(UTIL_PATH)/ubinize -m 2048 -p 128KiB -o runtime_data.ubi runtime_data.cfg && \
	$(UTIL_PATH)/ubinize -m 2048 -p 128KiB -o factory_data.ubi factory_data.cfg && \
	$(UTIL_PATH)/ubinize -m 2048 -p 128KiB -o backup_data.ubi backup_data.cfg && \
	$(FLASHMAKER) -p $(IMAGE_BUILD_DIR)/partition.conf -o $(PRODUCT_NAME) -i $(IMG_GEN) -d -b 128 $(FLASH_MAKE_EXOPT) && \
	find -name "*up.bin" -o -name "*flash*" |xargs mv -t ../
endif

else
	@echo "make $@.";
	cp -Rf $(MKIMAGE_PAH) $(IMAGE_INSTALL)/ ;
	cd $(IMAGE_INSTALL)/mkimage && \
	rm -rf ../*flash.bin ../*up.bin flashBuild.log output/ && \
	$(FLASHMAKER) -b build.ini -p ptnTbl.ini && \
	find ./output -name "*flash.bin" -o -name "*up.bin" |xargs mv -t ../ && \
	rm -rf ./output;
	@echo "make $@ done.";
endif

image_install:
	@echo "make $@ do nothing."

######################################### everything ###################################################
export everyrules = $(addprefix $(PID)_, config prep build clean install)
ifdef GPL_BUILD
export everything = $(EVERYTHING_ADDITIONAL) $(filter-out $(EVERYTHING_EXCLUDE), kernel modules dropbear rootfs)
else ifeq ($(NEED_COMPILE_SDK), y)
export everything = $(EVERYTHING_ADDITIONAL) $(filter-out $(EVERYTHING_EXCLUDE), kernel modules sdk wlan_header apps smb busybox ipv6_tools lldpd wireless rootfs)
else
export everything = $(EVERYTHING_ADDITIONAL) $(filter-out $(EVERYTHING_EXCLUDE), kernel modules wlan_header apps smb busybox ipv6_tools lldpd wireless rootfs)
endif

everything_config:
	@make apps_config busybox_config kernel_config

everything_prep:
	@make util toolchain
ifndef GPL_BUILD
	@make com_lib_prep
ifeq ($(NEED_COMPILE_SDK), y)
	@make sdk_prep
endif
endif
	@make everything_config
	@[ -d $(TOPDIR)/images/$(PID) ] || mkdir -p $(TOPDIR)/images/$(PID)
ifndef GPL_BUILD
	rm -rf $(TOPDIR)/images/$(PID)/*
	mkdir -p $(TOPDIR)/images/$(PID)/$(BOARD_TYPE)/rootfs
	cp -rf $(TOPDIR)/images/base/$(PID)/rootfs_base/* $(TOPDIR)/images/$(PID)/$(BOARD_TYPE)/rootfs/
	cp -rf $(TOPDIR)/images/base/$(PID)/$(BOARD_TYPE)  $(TOPDIR)/images/$(PID)/
endif
ifeq ($(CONFIG_MESH_SUPPORT), y)
ifneq ($(findstring $(BOARD_TYPE), board956x_eap245v3 board956x_eap245v3_ca board956x_eap245v3_jpspec board956x_eap265HD_v1 board956x_eap245v3_265HDv1_common board956x_eap265HD_v1_ca), )
	# MUST be placed last of everything_prep, to overwrite the fw in rootfs
	cp -rf $(TOPDIR)/images/base/$(PID)/rootfs_meshfw/eap245v3/rootfs/*  $(TOPDIR)/images/$(PID)/$(BOARD_TYPE)/rootfs/
endif
endif

everything_clean: $(addsuffix _clean, $(everything)) toolchain_clean
	#@make everything_config

everything_build: $(addsuffix _build, $(everything))
	@echo make $@ done.

everything_install: $(addsuffix _install, $(everything))
	@echo make $@ done.

everything_image: $(addsuffix _build, image)
	@echo make $@ done.

$(everyrules): $(PID)_% : everything_%

######################################### chip depended makefile and target #######################################
#
# TARGET may overwrite by config.$(BOARD) file.
#

export SMBAPPS_TARGET ?= ucm_lib eth_api_lib unix_sock_lib utility_lib ble_lib nvram_lib wlan_lib com_wrapper cloud_lib cloud-proc uclite msg-center enet_tool ecs_lib ecs tpsyslogd radius tddpd detect auth_time rssifilter rogueap gpiod pivot nvram_app ntpd wscheduler client-state tdp cli-server cli-client httpd_portal troubleshooting

ifneq ($(SDKAPPS),)
SMBAPPS_TARGET += $(SDKAPPS)
endif

ifeq ($(KERNELARCH), mips)
SMBAPPS_TARGET += tdb
endif

ifeq ($(DEBUG_MODE), y)
SMBAPPS_TARGET += unix_sock_cli
endif

ifeq ($(CONFIG_ROAMING_SUPPORT), y)
SMBAPPS_TARGET += eaproaming
endif

ifeq ($(CONFIG_MESH_SUPPORT), y)
SMBAPPS_TARGET += eapmesh
endif

ifeq ($(CONFIG_EAP_MONITOR_SUPPORT), y)
SMBAPPS_TARGET += eap-monitor
endif

ifeq ($(CONFIG_CHANNEL_DEPLOY_SUPPORT), y)
SMBAPPS_TARGET += channeldeploy
endif

ifeq ($(CONFIG_SMART_ANTENNA_SUPPORT), y)
SMBAPPS_TARGET += smartantenna
endif

ifeq ($(CONFIG_ENERGY_SAVE_SUPPORT), y)
SMBAPPS_TARGET += energysave
endif

ifeq ($(CONFIG_GRE_SUPPORT), y)
SMBAPPS_TARGET += tp_gre
endif

ifeq ($(CONFIG_JAPAN_DISASTER_MODE_SUPPORT), y)
SMBAPPS_TARGET += netcheck
endif

ifeq ($(KERNELVER), 4.4.60)
export SMBMODS_TARGET ?= gpiod_kernel tp_domain dhcp_capture tls_tuple wportal/portal vlan_manage rate_limit urlfilter 
else ifeq ($(KERNELVER), 5.4.164)
export SMBMODS_TARGET ?= gpiod_kernel tp_domain tls_tuple wportal/portal dhcp_capture vlan_manage rate_limit urlfilter 
else ifeq ($(KERNELVER), 5.4.213)
export SMBMODS_TARGET ?= gpiod_kernel tp_domain tls_tuple wportal/portal dhcp_capture vlan_manage rate_limit urlfilter 
else ifeq ($(KERNELVER), 3.3.8)
export SMBMODS_TARGET ?= gpiod_kernel pcie_reg tp_domain dhcp_capture tls_tuple wportal/portal vlan_manage rate_limit urlfilter 
else
export SMBMODS_TARGET ?= gpiod_kernel pcie_reg tp_domain ethernet_driver dhcp_capture tls_tuple wportal/portal vlan_manage rate_limit urlfilter 
endif

ifeq ($(CONFIG_MESH_SUPPORT), y)
SMBMODS_TARGET += mesh
endif

ifeq ($(CONFIG_RF_SCAN_SUPPORT), y)
SMBAPPS_TARGET += rfscan
endif

ifeq ($(CONFIG_WIDS_WIPS_SUPPORT), y)
SMBAPPS_TARGET += wids_wips
endif

ifeq ($(CONFIG_GRE_SUPPORT), y)
SMBMODS_TARGET += tp_gre
endif

ifeq ($(CONFIG_MULTIPORT_SUPPORT), y)
SMBAPPS_TARGET += dot1xAuth
SMBMODS_TARGET += multiPort
endif

SMBMODS_TARGET += tp_mdns
#
# sub makefiles
#
include Makefile.$(PID)
ifndef GPL_BUILD
include Makefile.smb_priv
endif
include Makefile.apps
ifndef GPL_BUILD
include qca/Makefile.wireless
endif

######################################### chip special rule #######################################
export ENETDIR           = $(SMBMOD_PATH)/ethernet_driver
export ENET_TOOL_PATH    = $(ENETDIR)/tools

# re-write ethernet_driver install rule, 
# because it's install directory is diffrent from others.
ethernet_driver.ko_clean:
ethernet_driver.ko_build:
ethernet_driver.ko_install:
	@$(call indir, $(SMBMOD_PATH)/ethernet_driver, \
		cp -f athrs_gmac.ko $(SMB_MOD_INSTALL)/ag7240_mod.ko)

eth_api_lib_clean:
	@$(call indir, $(ENETDIR), $(CROSSMAKE) libeth_api_clean);

eth_api_lib_build:
	@$(call indir, $(ENETDIR), $(CROSSMAKE) libeth_api.a);
	@make eth_api_lib_install;

eth_api_lib_install:
	@$(call indir, $(ENETDIR), cp -f libeth_api.a $(SMB_LIB_INSTALL));

enet_tool_clean:
	@$(call indir, $(ENET_TOOL_PATH), $(CROSSMAKE) KERNELPATH=$(LINUX_KERNEL_PATH) TOOLPATH=$(LIBC_PATH)/$(TOOLARCH) clean);

enet_tool_build:
	@$(call indir, $(ENET_TOOL_PATH), $(CROSSMAKE) KERNELPATH=$(LINUX_KERNEL_PATH) TOOLPATH=$(LIBC_PATH)/$(TOOLARCH));

enet_tool_install:
	@$(call indir, $(ENET_TOOL_PATH), cp -f ethreg $(SMB_APP_INSTALL));

######################################### special phy rules #######################################
export ssdk_shell_path = $(QCA_SDK_ROOT_PATH)/qca-ssdk-shell
export aq-fw-dl_path = $(QCA_SDK_ROOT_PATH)/aq-fw-download

QCASSDK_SHELL_OPTS=TOOL_PATH=$(TOOLCHAIN_INSTALL)/$(TOOLCHAIN_PACKAGE)/bin \
		SYS_PATH=$(LINUX_KERNEL_PATH) \
		KVER=$(KERNELVER) \
		ARCH=$(KERNELARCH) \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)"

ssdk_shell_clean:
	@$(call indir, $(ssdk_shell_path), $(CROSSMAKE) clean);

ssdk_shell_build:
	@$(call indir, $(ssdk_shell_path), $(CROSSMAKE) $(strip $(QCASSDK_SHELL_OPTS)));
	@make ssdk_shell_install;

ssdk_shell_install:
	$(call indir, $(ssdk_shell_path), cp -f build/bin/ssdk_sh $(SBIN_INSTALL));

aq_fw_dl_clean:
	@$(call indir, $(aq-fw-dl_path), $(CROSSMAKE) clean);

aq_fw_dl_build:
	@$(call indir, $(aq-fw-dl_path), $(CROSSMAKE));
	@make aq_fw_dl_install;

aq_fw_dl_install:
	$(call indir, $(aq-fw-dl_path), cp -f aq-fw-download $(SBIN_INSTALL));

######################################### chip specified cflags ###################################
# for smb_priv app usage
ifneq ($(findstring $(CONFIG_TP_QCA_SDK_VERSION_CODE),$(TP_SPF_10_0_0) $(TP_SPF_11_1_0) $(TP_SPF_11_3_0) $(TP_SPF_11_4_0) $(TP_SPF_12_1_0) $(TP_SPF_12_2_0) $(TP_SPF_12_4_0)), )
export DRIVER_CFLAGS = -I$(SDK_INSTALL)/usr/include
else
export DRIVER_CFLAGS = -I$(WLAN_PATH)/include -I$(WLAN_PATH)/os/linux/include -I$(WLAN_PATH)/lmac/ath_dev -I$(WLAN_PATH)/os/linux/src -I$(WLAN_PATH)/umac/include
endif

