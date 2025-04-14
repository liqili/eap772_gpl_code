#
# ipv6 tools Makefile
# 

export Q=@
export INSTALL_ROOT := `pwd`/install
export IPV6_LIB_INSTALL = $(INSTALL_ROOT)/lib
export IPV6_APPS_INSTALL = $(INSTALL_ROOT)/usr/bin
export IPV6MAKE:=$(MAKE) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld


#IP6_TOOLS_CONFIG=radvd
#IP6_TOOLS_NOCONF=ip_2.6.33 dhcpv6 ipcserver dproxy_nexgen iputils radvdCtl
IP6_TOOLS_NOCONF=dhcpv6
IP6_TOOLS_TARGET=$(filter-out $(IP6_TOOLS_EXECLUDE), $(IP6_TOOLS_CONFIG) $(IP6_TOOLS_NOCONF)) $(IP6_TOOLS_ADDITIONAL)

all: ipv6_tools_build ipv6_tools_install

mcast_tools_config:
	$(Q)cd mcast-tools && ./configure 

mcast_tools_clean:
	$(Q)cd mcast-tools && if test -f Makefile; then $(IPV6MAKE) clean; fi;

mcast_tools_build:
	$(Q)cd mcast-tools && $(IPV6MAKE)

mcast_tools_install:
	$(Q)cd mcast-tools && cp -f mcast-tools $(IPV6_APPS_INSTALL)

ip_2.6.31_clean:
	$(Q)cd iproute2-2.6.31 && $(IPV6MAKE) clean

ip_2.6.31_build:
	$(Q)cd iproute2-2.6.31 && $(IPV6MAKE)

ip_2.6.31_install:
	$(Q)cd iproute2-2.6.31 && cp -f ip/ip $(IPV6_APPS_INSTALL)
	
libnl-1.1_config:
	$(Q)cd libnl-1.1 && chmod +x ./configure && \
	./configure --prefix=$(INSTALL_ROOT) --build=i686-linux --host=$${TOOLPREFIX%-} CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar

libnl-1.1_clean:
	$(Q)cd libnl-1.1 && if test -f Makefile; then $(IPV6MAKE) clean; fi;

libnl-1.1_build:
	$(Q)cd libnl-1.1 && $(IPV6MAKE)
	
libnl-1.1_install:
	$(Q)cd libnl-1.1 && $(IPV6MAKE) install

libopenssl_config:
	$(Q)cd openssl-1.0.0d && chmod +x ./Configure && \
	./Configure os/compiler:$(TOOLPREFIX)gcc --prefix=$(INSTALL_ROOT) \
	--build=i686-linux --host=$${TOOLPREFIX%-} CC=$(TOOLPREFIX)gcc CXX=$(TOOLPREFIX)g++ AR=$(TOOLPREFIX)ar

libopenssl_clean:
	$(Q)cd openssl-1.0.0d && if test -f Makefile; then $(IPV6MAKE) clean; fi;

libopenssl_build:
	cd openssl-1.0.0d && $(IPV6MAKE)

libopenssl_install:
	$(Q)cd openssl-1.0.0d && $(IPV6MAKE) install

ip_2.6.33_clean:
	$(Q)cd iproute2-2.6.33 && $(IPV6MAKE) clean

ip_2.6.33_build:
	$(Q)cd iproute2-2.6.33 && $(IPV6MAKE)

ip_2.6.33_install:
	$(Q)cd iproute2-2.6.33 && cp -f ip/ip $(IPV6_APPS_INSTALL)

dproxy_nexgen_clean:
	$(Q)cd dproxy-nexgen && $(IPV6MAKE) clean

dproxy_nexgen_build:
	$(Q)cd dproxy-nexgen && $(IPV6MAKE)

dproxy_nexgen_install:
	$(Q)cd dproxy-nexgen && cp -f dnsproxy $(IPV6_APPS_INSTALL)

dhcpv6_clean:
	$(Q)cd wide-dhcpv6 && $(IPV6MAKE) clean

dhcpv6_build:
	$(Q)cd wide-dhcpv6 && $(IPV6MAKE)

dhcpv6_install:
	$(Q)cd wide-dhcpv6 && cp -f dhcp6c $(IPV6_APPS_INSTALL)
	$(Q)cd $(INSTALL_ROOT)/etc && \
		/usr/bin/openssl rand -base64 16 > dhcp6sctlkey &&	\
		/usr/bin/openssl rand -base64 16 > dhcp6cctlkey

radvd_config:
	$(Q)cd radvd-1.7 && ./configure --host=$${TOOLPREFIX%-} CFLAGS="-Dyywrap\(...\)=1 -DYY_SKIP_YYWRAP"

radvd_clean:
	$(Q)cd radvd-1.7 && if test -f Makefile; then $(IPV6MAKE) clean; fi;

radvd_build:
	$(Q)cd radvd-1.7 && $(IPV6MAKE)

radvd_install:
	$(Q)cd radvd-1.7 && cp -f radvd $(IPV6_APPS_INSTALL)

radvdCtl_clean:
	$(Q)cd radvdCtl && $(IPV6MAKE) clean
	
radvdCtl_build:
	$(Q)cd radvdCtl && $(IPV6MAKE)
	
radvdCtl_install:
	$(Q)cd radvdCtl && cp -f radvdctl $(IPV6_APPS_INSTALL)

ipcserver_clean:
	$(Q)cd ipcserver && $(IPV6MAKE) clean

ipcserver_build:
	$(Q)cd ipcserver && $(IPV6MAKE)

ipcserver_install:
	$(Q)cd ipcserver && cp -f ipcserver $(IPV6_APPS_INSTALL)

iputils_clean:
	$(Q)cd iputils && $(IPV6MAKE) clean

iputils_build:
	$(Q)cd iputils && $(IPV6MAKE)

iputils_install:
	$(Q)cd iputils && cp -f ping6 $(IPV6_APPS_INSTALL)

config: $(addsuffix _config, $(IP6_TOOLS_CONFIG))
	$(Q)mkdir -p $(INSTALL_ROOT)
	$(Q)mkdir -p $(IPV6_LIB_INSTALL)
	$(Q)mkdir -p $(IPV6_APPS_INSTALL)
	$(Q)echo make $@ done

clean: $(addsuffix _clean, $(IP6_TOOLS_TARGET))
	$(Q)echo make $@ done

build: $(addsuffix _build, $(IP6_TOOLS_TARGET))
	$(Q)echo make $@ done

install: $(addsuffix _install, $(IP6_TOOLS_TARGET))
	$(Q)echo make $@ done
	
rebuild: clean build install
	$(Q)echo "rebuilt <$@> at `date`."

%: %_build %_install
	$(Q)echo "built and installed <$@> at `date`."

%_rebuild: %_clean %_build %_install
	$(Q)echo "rebuilt and installed <$@> at `date`."
