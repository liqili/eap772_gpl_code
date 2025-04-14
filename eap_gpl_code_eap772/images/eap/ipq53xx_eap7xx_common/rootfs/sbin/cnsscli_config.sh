#!/bin/sh

#enable CBC for MIAMI
echo "====enable cold boot for integrated===="
cnsscli -i integrated   --enable_cold_boot_support 1 > /dev/null

#enable CBC for WKK
echo "====enable cold boot for qcn9224_pci1===="
cnsscli -i qcn9224_pci1 --enable_cold_boot_support 1 > /dev/null

