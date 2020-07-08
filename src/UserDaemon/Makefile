# Makefile
.PHONY:clean

#################################
#	daemon on user space	    #
#################################

sofa_deploy_dir:="/usr/sofa/"

SOFA_C_MYFLAGS = -DSOFA_USER_SPACE=1
SOFA_D_CFLAGS= -Wall -lpthread -DSOFA_USER_SPACE=1
SOFA_D_INCLUDE= -I/usr/include

SOFA_D_SRCS = ../common/sofa_mm.c ../common/sofa_config.c lfsm_req_handler.c userD_req_mgr.c userD_sock_srv.c calculate_perf.c userD_main.c
SOFA_D_OBJS = sofa_mm.o sofa_config.o lfsm_req_handler.o userD_req_mgr.o userD_sock_srv.o calculate_perf.o userD_main.o

all:
	@echo "Ready to build sofa user daemon"
	gcc -c $(SOFA_D_SRCS) $(SOFA_D_INCLUDE) $(SOFA_C_MYFLAGS)
	gcc -o "sofa_daemon" $(SOFA_D_OBJS) $(SOFA_D_CFLAGS)	

install: all
	if [ -d $(sofa_deploy_dir) ]; then \
		rm -rf $(sofa_deploy_dir)/sofa_daemon; \
	else \
		mkdir -p $(sofa_deploy_dir); \
	fi	
	cp sofa_daemon $(sofa_deploy_dir)
	
clean:
	@echo "Ready to clean sofa user daemon object files"
	rm -rf *.o
