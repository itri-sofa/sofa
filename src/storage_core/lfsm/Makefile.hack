obj-m := hackdr.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
EXTRA_CFLAGS := -DOUTSIDE_ATTACH_LPN=1 -DWF_NONBLOCKING_IO=1 -DWF_PROCESS_NOWAIT=1 -DSKIP_CONFLICT_BIO=0 -DSSD_SIZE_IN_SEU=96 -DHDD_SIZE_IN_SEU=0 #-DPSEUDO=1 #for hand GC
default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
	gcc rfsioctl.c -o rfsioctl; 

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
	rm -rf Module.markers modules.order Module.symvers

hackdr-objs :=  autoflush.o bflush.o sflush.o mqbiobox.o lfsm.o GC.o io_write.o io_read.o \
		io_common.o bmt.o bmt_update_log.o bmt_commit.o ioctl.o \
		EU_create.o EU_access.o New_Super_map.o New_Dedicated_map.o  \
		special_cmd.o spare_bmt_ul.o metadatapool.o biocbh.o active_hash.o\
		perf_measure.o err_manager.o bmt_ppdm.o dbmtapool.o cmdlog.o lfsm_test.o timelog.o \
		procfs.o  rmutex.o erase_count.o devio.o diskgrp.o batchread.o hpeu.o hpeu_recov.o \
		persistlog.o ecc.o hpeu_gc.o xorbmp.o eccpri.o stripe.o pgroup.o metabioc.o \
		pgcache.o erase_manager.o mdnode.o conflict.o metalog.o \
		spblk.o batchitem.o threadqu.o threadqu_batch.o sysapi.o xorcalc.o
