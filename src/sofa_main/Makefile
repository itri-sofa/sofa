# Makefile
.PHONY:clean

sofa_deploy_dir:="/usr/sofa/"

#################################
#	ccmablk  objs			    #
#################################
sofa-objs := ../common/sofa_common.o ../common/sofa_mm.o ../common/sofa_config.o
sofa-objs += ../common/sofa_radix_tree.o

sofa-objs += ../sofa_feature_generic/io_common/storage_api_wrapper.o
sofa-objs += drv_lfsmcmd_handler.o drv_chrdev.o sofa_main.o sofa_module_init.o

obj-m := sofa.o

all:
	@echo "Ready to build sofa module"
	$(MAKE) -C/lib/modules/"$(shell uname -r)"/build M=$(PWD) modules

install: all
	if [ -d $(sofa_deploy_dir) ]; then \
		rm -rf $(sofa_deploy_dir)/sofa.ko; \
	else \
		mkdir -p $(sofa_deploy_dir); \
	fi	
	cp sofa.ko $(sofa_deploy_dir)

clean:
	@echo "Ready to clean sofa modules"
	$(MAKE) -C/lib/modules/"$(shell uname -r)"/build M=$(PWD) clean
	rm -rf *.o *.ko *.symvers *.mod.c .*.cmd Module.markers modules.order .tmp_versions version.h
	rm -rf ../common/*.o
	rm -rf ../sofa_feature_generic/*.o
	rm -rf ../sofa_feature_generic/io_common/*.o
