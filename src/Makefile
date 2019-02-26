#############################################################
# friso chinese word segmentation makefile.		    #
#		do not use it for commercial use.	    #
# @author	chenxin 				    #
# @email	chenxin619315@gmail.com   		    #
#############################################################

#complie
CC = gcc
C++ = g++
#include directory
INCLUDE = .
#complie flags for devolep
CFLAGS = -g -Wall
#complile flags for products
FFLAGS = -O2 -Wall -fPIC
#FFLAGS = -g -Wall -fPIC
#extension libs for friso
ELIB = m
SQL_LIB = mysqlclient
SQL_LIB_DIR = /usr/lib64/mysql
LIB_FILE = libfriso.so
STA_FILE = libfriso.a
LIBRARY_DIR = /usr/lib
INCLUDE_DIR = /usr/include/friso
INSTALL_DIR = /usr/local/bin


OBJECT = friso.o friso_array.o friso_hash.o friso_lexicon.o friso_link.o friso_string.o friso_ctype.o friso_UTF8.o friso_GBK.o
SOURCE = friso_ctype.c friso_hash.c friso_UTF8.c friso_lexicon.c friso_array.c friso_GBK.c friso_link.c friso.c friso_string.c

all: share friso

static: $(OBJECT)
	ar -cr $(STA_FILE) $(OBJECT)

share: $(OBJECT)
	$(CC) $(FFLAGS) $(OBJECT) -fPIC -shared -l$(ELIB) -lpthread -o $(LIB_FILE)

##debug: $(SOURCE)
##    $(CC) $(CFLAGS) $(SOURCE) -o friso

friso: fy-friso.o socket_server.o pinyin.o nlp_log.o
	$(C++) fy-friso.o socket_server.o pinyin.o nlp_log.o  -o ./friso -L. L $(SQL_LIB_DIR) -lfriso -lzlog -l$(SQL_LIB)

fy-friso.o: friso_API.h friso.h fy-friso.c
	$(CC) $(FFLAGS) -c fy-friso.c

friso.o: friso.c friso.h friso_API.h
	$(CC) $(FFLAGS) -c friso.c -l$(ELIB)

friso_array.o: friso_array.c friso_API.h
	$(CC) $(FFLAGS) -c friso_array.c

friso_hash.o: friso_hash.c friso_API.h
	$(CC) $(FFLAGS) -c friso_hash.c

friso_lexicon.o: friso_hash.c friso_lexicon.c friso_API.h friso.h
	$(CC) $(FFLAGS) -c friso_lexicon.c -L $(SQL_LIB_DIR) -l$(SQL_LIB)

friso_link.o: friso_link.c friso_API.h
	$(CC) $(FFLAGS) -c friso_link.c

friso_string.o: friso_string.c friso_API.h
	$(CC) $(FFLAGS) -c friso_string.c

friso_ctype.o: friso_API.h friso_ctype.h friso_ctype.c
	$(CC) $(FFLAGS) -c friso_ctype.c

friso_UTF8.o: friso_API.h friso_ctype.h friso_UTF8.c
	$(CC) $(FFLAGS) -c friso_UTF8.c

friso_GBK.o: friso_API.h friso_ctype.h friso_GBK.c
	$(CC) $(FFLAGS) -c friso_GBK.c

socket_server.o: socket_server.c socket_server.h
	$(CC) $(FFLAGS) -c socket_server.c

pinyin.o: pinyin.c pinyin.h
	$(CC) $(FFLAGS) -c pinyin.c

nlp_log.o: nlp_log.c nlp_log.h
	$(CC) $(FFLAGS) -c nlp_log.c
#clean all the object files.
.PHONY: clean
clean:
	find . -name \*.so | xargs rm -f
	find . -name \*.o  | xargs rm -f
	@if [ -f friso ];\
	    then\
	    rm -f friso;\
	fi

#install friso
install: friso
	@if [ -d $(INSTALL_DIR) ] && [ -d $(LIBRARY_DIR) ];\
	    then\
	    cp friso $(INSTALL_DIR);\
	    chmod a+x $(INSTALL_DIR)/friso;\
	    chmod og-w $(INSTALL_DIR)/friso;\
	    cp $(LIB_FILE) $(LIBRARY_DIR);\
	    chmod a+x $(LIBRARY_DIR)/$(LIB_FILE);\
	    chmod og-w $(LIBRARY_DIR)/$(LIB_FILE);\
	    echo "install friso successfully.";\
	    echo "Usage: friso -init friso configuration file path.";\
	    else\
	    echo "Sorry, $(INSTALL_DIR) or $(LIBRARY_DIR) does not exits.";\
	fi
	@if [ ! -d $(INCLUDE_DIR) ];\
	    then\
	    mkdir $(INCLUDE_DIR);\
	fi
	@cp *.h $(INCLUDE_DIR);\
	chmod a+r $(INCLUDE_DIR)/*.h;\
	chmod a+x $(INCLUDE_DIR)/*.h;
