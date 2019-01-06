
PIDGIN_TREE_TOP ?= ../pidgin-2.10.11
LIBPURPLE_DIR ?= $(PIDGIN_TREE_TOP)/libpurple
WIN32_DEV_TOP ?= $(PIDGIN_TREE_TOP)/../win32-dev

WIN32_CC ?= $(WIN32_DEV_TOP)/mingw-4.7.2/bin/gcc

PROTOC_C ?= protoc-c
PKG_CONFIG ?= pkg-config

DIR_PERM = 0755
LIB_PERM = 0755
FILE_PERM = 0644

# Note: Use "-C .git" to avoid ascending to parent dirs if .git not present
GIT_REVISION_ID = $(shell git -C .git rev-parse --short HEAD 2>/dev/null)
REVISION_ID = $(shell hg id -i 2>/dev/null)
REVISION_NUMBER = $(shell hg id -n 2>/dev/null)
ifneq ($(REVISION_ID),)
PLUGIN_VERSION ?= 0.9.$(shell date +%Y.%m.%d).git.r$(REVISION_NUMBER).$(REVISION_ID)
else ifneq ($(GIT_REVISION_ID),)
PLUGIN_VERSION ?= 0.9.$(shell date +%Y.%m.%d).git.$(GIT_REVISION_ID)
else
PLUGIN_VERSION ?= 0.9.$(shell date +%Y.%m.%d)
endif

CFLAGS	?= -O2 -g -pipe
LDFLAGS ?= -Wl,-z,relro

CFLAGS  += -std=gnu99 

# Comment out to disable localisation
CFLAGS += -DENABLE_NLS

# Do some nasty OS and purple version detection
ifeq ($(OS),Windows_NT)
  #only defined on 64-bit windows
  PROGFILES32 = ${ProgramFiles(x86)}
  ifndef PROGFILES32
    PROGFILES32 = $(PROGRAMFILES)
  endif
  PPL_TARGET = php_plugin_loader.dll
  PPL_DEST = "$(PROGFILES32)/Pidgin/plugins"
else
  UNAME_S := $(shell uname -s)

  #.. There are special flags we need for OSX
  ifeq ($(UNAME_S), Darwin)
    #
    #.. /opt/local/include and subdirs are included here to ensure this compiles
    #   for folks using Macports.  I believe Homebrew uses /usr/local/include
    #   so things should "just work".  You *must* make sure your packages are
    #   all up to date or you will most likely get compilation errors.
    #
    INCLUDES = -I/opt/local/include -lz $(OS)

    CC = gcc
  else
    CC ?= gcc
  endif

  ifeq ($(shell $(PKG_CONFIG) --exists purple 2>/dev/null && echo "true"),)
    PPL_TARGET = FAILNOPURPLE
    PPL_DEST =
  else
    PPL_TARGET = php_plugin_loader.so
    PPL_DEST = $(DESTDIR)`$(PKG_CONFIG) --variable=plugindir purple`
  endif
endif

WIN32_CFLAGS = -std=gnu99 -I$(WIN32_DEV_TOP)/glib-2.28.8/include -I$(WIN32_DEV_TOP)/glib-2.28.8/include/glib-2.0 -I$(WIN32_DEV_TOP)/glib-2.28.8/lib/glib-2.0/include -DENABLE_NLS -Wall -Wextra -Wno-unused-but-set-parameter -Wno-unused-but-set-variable -Wno-deprecated-declarations -Wno-unused-parameter -fno-strict-aliasing -Wformat
WIN32_LDFLAGS = -L$(WIN32_DEV_TOP)/glib-2.28.8/lib -lpurple -lintl -lglib-2.0 -lgobject-2.0 -g -ggdb -static-libgcc -lz
WIN32_PIDGIN2_CFLAGS = -I$(PIDGIN_TREE_TOP)/libpurple -I$(PIDGIN_TREE_TOP) $(WIN32_CFLAGS)
WIN32_PIDGIN2_LDFLAGS = -L$(PIDGIN_TREE_TOP)/libpurple $(WIN32_LDFLAGS)

CFLAGS += -DLOCALEDIR=\"$(LOCALEDIR)\"

C_FILES := 
PURPLE_COMPAT_FILES :=
PURPLE_C_FILES := $(C_FILES) php-plugin-loader.c

.PHONY:	all install FAILNOPURPLE clean install-icons install-locales %-locale-install

LOCALES = $(patsubst %.po, %.mo, $(wildcard po/*.po))

all: $(PPL_TARGET)

php_plugin_loader.so: $(PURPLE_C_FILES) $(PURPLE_COMPAT_FILES)
	$(CC) -fPIC $(CFLAGS) $(CPPFLAGS) -shared -o $@ $^ $(LDFLAGS) `$(PKG_CONFIG) purple glib-2.0 json-glib-1.0 --libs --cflags`  $(INCLUDES) -Ipurple2compat -g -ggdb

php_plugin_loader.dll: $(PURPLE_C_FILES) $(PURPLE_COMPAT_FILES)
	$(WIN32_CC) -O0 -g -ggdb -shared -o $@ $^ $(WIN32_PIDGIN2_CFLAGS) $(WIN32_PIDGIN2_LDFLAGS) -Ipurple2compat

install: $(PPL_TARGET) 
	mkdir -m $(DIR_PERM) -p $(PPL_DEST)
	install -m $(LIB_PERM) -p $(PPL_TARGET) $(PPL_DEST)


FAILNOPURPLE:
	echo "You need libpurple development headers installed to be able to compile this plugin"

clean:
	rm -f $(PPL_TARGET)

gdb:
	gdb --args pidgin -c ~/.fake_purple -n -m

