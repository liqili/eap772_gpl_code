1. 制作云固件的命令：./make_flash -p partition.conf -o "m5-cloud" -c cloudinfo.conf -s "V1.0.0P1" -d

-c 选项是针对云固件添加的选项，参数cloudinfo.conf是相应的配置文件，制作固件时应将它拷入对应的目录中。

cloudinfo.conf是按照json格式编写的，目前有两个参数，fw_id和hw_list 。 
fw_id项若为空，则该ID值由makeflash工具自动生成并填充到固件相应位置，若不为空，则由该值指定。
hw_list项目前未使用。


2. make_flah.c中有一个参数 cloudOn 
    1）cloudOn设为TRUE时，生成云固件，需要进行rsa签名才能升级
	2) cloudOn设为FALSE时，生成一般固件，不需要进行rsa签名就可以升级，升级成一般固件
	3）目前nvrammanager从一般固件不能直接升级成云固件，需要烧flash（主要是soft-version分区格式不一样，云升级要检查该分区）。