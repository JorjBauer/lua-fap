# Linux (Debian Lenny)
#CFLAGS=-g -O2 -fpic -I/usr/include/lua5.1
#LDFLAGS=-O -shared -fpic -lfap
#LUAPATH=/usr/share/lua/5.1
#CPATH=/usr/lib/lua/5.1

# MacOS
CFLAGS=-g -Wall -O2
LDFLAGS=-bundle -undefined dynamic_lookup -lfap
MACOSX_VERSION=10.5
LUAPATH=/usr/local/share/lua/5.1
CPATH=/usr/local/lib/lua/5.1

#########################################################
#
# YOU SHOULD NOT HAVE TO CHANGE ANYTHING BELOW THIS LINE.
# If you do, then send me email letting me know what and 
# why!
# -- Jorj Bauer <jorj@jorj.org>
#
#########################################################

BRANCH_VERSION=.branch_version
BUILD_VERSION=.build_version
TARGET=fap.so
OBJS=fap.o luaabstract.o

all: $(TARGET)

install: $(TARGET)
	cp $(TARGET) $(CPATH)

clean:
	rm -f *.o *.so *~

distclean: clean
	rm -f $(BUILD_VERSION) $(BRANCH_VERSION)

$(TARGET): version $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -DVERSION="\"$$(cat VERSION).$$(cat $(BRANCH_VERSION))-$$(cat $(BUILD_VERSION))\"" -fno-common -c $< -o $@

# Dependencies
fap.c: luaabstract.c luaabstract.h

luaabstract.c: luaabstract.h

# build_version stuff
.PHONY: version branch_version

version:
	@if ! test -f $(BUILD_VERSION); then echo 0 > $(BUILD_VERSION); fi
	@echo $$(($$(cat $(BUILD_VERSION)) + 1)) > $(BUILD_VERSION)
	@if ! test -f $(BRANCH_VERSION); then git log --pretty=oneline -1|cut -c1-8 > $(BRANCH_VERSION); fi

