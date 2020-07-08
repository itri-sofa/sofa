# Makefile
.PHONY:clean

sofa_deploy_dir:="/usr/sofa/"

obj-m := lfsmdr.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
EXTRA_CFLAGS := -DOUTSIDE_ATTACH_LPN=1 -DWF_NONBLOCKING_IO=1 -DWF_PROCESS_NOWAIT=1 -DSKIP_CONFLICT_BIO=0 -DSSD_SIZE_IN_SEU=0 -DHDD_SIZE_IN_SEU=0 -g
default:
	@echo "Ready to build sofa lfsm module"
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
	gcc rfsioctl.c -o rfsioctl; 
	gcc lfsmfsck.c -o lfsmfsck;
	gcc lfsm_set_mem.c -o lfsm_set_mem

autotest1372659817293711890273: default
	
# 	echo $(PWD)/scandev > /proc/sys/kernel/hotplug ; \
#	echo -e "#!/bin/sh" > $(PWD)/scandev; \
#	echo -e "$(PWD)/rfsioctl /dev/lfsm scandev" >> $(PWD)/scandev;  


clean:
	@echo "Ready to clean sofa lfsm modules"
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
	rm -rf *.o *.ko *.symvers *.mod.c .*.cmd Module.markers modules.order Module.symvers

hotplug: 
	echo $(PWD)/scandev > /proc/sys/kernel/hotplug   		

lfsmdr-objs :=  common/tools.o common/mem_manager.o common/rmutex.o
lfsmdr-objs +=  config/lfsm_cfg.o
lfsmdr-objs +=  io_manager/io_read.o io_manager/io_write.o io_manager/io_common.o
lfsmdr-objs +=  io_manager/mqbiobox.o io_manager/biocbh.o
lfsmdr-objs +=  system_metadata/Super_map.o system_metadata/Dedicated_map.o
lfsmdr-objs +=  sflush.o lfsm_blk_dev.o lfsm_core.o GC.o \
		bmt.o bmt_commit.o ioctl.o EU_create.o EU_access.o \
		special_cmd.o spare_bmt_ul.o \
		err_manager.o bmt_ppdm.o dbmtapool.o lfsm_test.o \
		lfsm_info.o  erase_count.o devio.o diskgrp.o batchread.o hpeu.o hpeu_recov.o \
		persistlog.o ecc.o hpeu_gc.o xorbmp.o stripe.o pgroup.o metabioc.o \
		pgcache.o erase_manager.o mdnode.o conflict.o metalog.o \
		sysapi_linux_kernel.o xorcalc.o hotswap.o\
		bio_ctrl.o nextprepare.o hook.o lfsm_thread_monitor.o \
		perf_monitor/thd_perf_mon.o perf_monitor/ioreq_pendstage_mon.o \
		perf_monitor/ssd_perf_mon.o \
		perf_monitor/calculate_iops.o \
		lfsm_drv_main.o lfsm_module_init.o
		
install: default
	if [ -d $(sofa_deploy_dir) ]; then \
		rm -rf $(sofa_deploy_dir)/lfsmdr.ko; \
	else \
		mkdir -p $(sofa_deploy_dir); \
	fi	
	cp lfsmdr.ko /usr/sofa/


