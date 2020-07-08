# Makefile
.PHONY:clean

#################################
#	daemon on user space	    #
#################################

sofa_deploy_dir:="/usr/sofa/"

SOFA_D_INCLUDE= -I/usr/include

SOFA_D_CFLAGS= -Wall -lpthread -ljson-c


SOFA_D_SRCS = sock_util.c lfsmcmd_parser.c usercmd_parser.c \
			configcmd_parser.c configcmd_handler.c \
			lfsmcmd_handler.c usercmd_handler.c \
			sofa_adm_main.c
 
SOFA_D_OBJS = sock_util.o lfsmcmd_parser.o usercmd_parser.o \
			configcmd_parser.o configcmd_handler.o \
			lfsmcmd_handler.o usercmd_handler.o \
			sofa_adm_main.o
 

all:
	@echo "Ready to build sofa adm tools\n"
	gcc -c $(SOFA_D_SRCS) $(SOFA_D_INCLUDE)
	gcc -o "sofaadm" $(SOFA_D_OBJS) $(SOFA_D_CFLAGS)	

install: all
	if [ -d $(sofa_deploy_dir) ]; then \
		rm -rf $(sofa_deploy_dir)/bin/sofaadm; \
	else \
		mkdir -p $(sofa_deploy_dir)/bin/; \
	fi	
	cp sofaadm $(sofa_deploy_dir)/bin/
	
clean:
	@echo "Ready to clean sofa adm tools object files\n"
	rm -rf *.o