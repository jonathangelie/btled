BIN_NAME := btled
VERSION_MAJOR=1
VERSION_MINOR=0

BLUEZVERSION=5.48
BLUEZSERVER=https://git.kernel.org/pub/scm/bluetooth/bluez.git/snapshot/
BLUEZDIR=../bluez-$(BLUEZVERSION)
#BLUEZDIR=/home/jonathan/projects/abeeway-bluepy/bluepy/bluez-$(BLUEZVERSION)
SRCDIR   = ../src
HEADERDIR= ../inc
OBJDIR   = ../obj
BINDIR   = ../bin
PKGDIR   = ../pkg

OUTPUT_FILENAME := $(BIN_NAME)

export OUTPUT_FILENAME
MAKEFILE_NAME := $(MAKEFILE_LIST)
MAKEFILE_DIR := $(dir $(MAKEFILE_NAME) )

#GNU_PREFIX := arm-none-eabi-

MK := mkdir
RM := rm -rf

#echo suspend
ifeq ("$(VERBOSE)","1")
NO_ECHO :=
else
NO_ECHO := @
endif

# Toolchain commands
CC       := "$(GNU_PREFIX)gcc"
AS       := "$(GNU_PREFIX)as"
AR       := "$(GNU_PREFIX)ar" -r
LD       := "$(GNU_PREFIX)ld"
NM       := "$(GNU_PREFIX)nm"
OBJDUMP  := "$(GNU_PREFIX)objdump"
OBJCOPY  := "$(GNU_PREFIX)objcopy"
SIZE     := "$(GNU_PREFIX)size"

#function for removing duplicates in a list
remduplicates = $(strip $(if $1,$(firstword $1) $(call remduplicates,$(filter-out $(firstword $1),$1))))

-include bluez.mk

#sources project
SOURCES := $(wildcard $(SRCDIR)/*.c)
SOURCES += $(BLUEZ_SRC)

#includes common to all targets
INC_PATHS += -I$(BLUEZDIR)
INC_PATHS += -I$(HEADERDIR)
#INC_PATHS += -I$(BLUEZDIR)/attrib -I$(BLUEZDIR) -I$(BLUEZDIR)/lib -I$(BLUEZDIR)/src -I$(BLUEZDIR)/gdbus -I$(BLUEZDIR)/btio

LISTING_DIRECTORY = $(OBJDIR)

# Sorting removes duplicates
BUILD_DIRECTORIES := $(sort $(OBJDIR) $(BINDIR) $(LISTING_DIRECTORY) )


CFLAGS += --std=gnu99
CFLAGS += -Os -g -Wall -Werror
CFLAGS += -DHAVE_CONFIG_H 
CFLAGS += -DBLUEPY_DEBUG
CFLAGS += `pkg-config --cflags glib-2.0`


#Link Library
LIBS += $(BLUEZDIR)/lib/.libs/libbluetooth-internal.a
LIBS += $(BLUEZDIR)/src/.libs/libshared-glib.a
LIBS += -lreadline
LIBS += `pkg-config --libs glib-2.0`


#default target - first one defined
default:  $(BIN_NAME)

#building all targets
all:
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e $(PROJECT_NAME)
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e package

#target for printing all targets
help:
	@echo following targets are available:
	@echo 	debug release


C_SOURCE_FILE_NAMES = $(notdir $(SOURCES))
C_PATHS = $(call remduplicates, $(dir $(SOURCES) ) )
C_OBJECTS = $(addprefix $(OBJDIR)/, $(C_SOURCE_FILE_NAMES:.c=.o) )

vpath %.c $(C_PATHS)
#vpath %.s $(ASM_PATHS)

OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS)

CFLAGS += -DDEBUG
CFLAGS += -ggdb3 -O0
ASMFLAGS += -DDEBUG -ggdb3 -O0
LDFLAGS += -ggdb3 -O0

$(BIN_NAME): $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo LD $(OUTPUT_FILENAME)
	@$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(BINDIR)/$(OUTPUT_FILENAME)

## Create build directories
$(BUILD_DIRECTORIES):
	$(MK) $@

# Create objects from C SRC files
$(OBJDIR)/%.o: %.c
	@echo CC $(notdir $<)
	$(NO_ECHO)$(CC) $(CFLAGS) $(INC_PATHS) -c -o $@ $<

# Link
$(BINDIR)/$(OUTPUT_FILENAME): $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo Linking target: $(OUTPUT_FILENAME)
	$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(BINDIR)/$(OUTPUT_FILENAME)

package_prepare:
	@$(RM) $(PKGDIR)
	@mkdir $(PKGDIR)
	@mkdir $(PKGDIR)/$(BIN_NAME);
	@mkdir $(PKGDIR)/$(BIN_NAME)/DEBIAN;
	@mkdir $(PKGDIR)/$(BIN_NAME)/usr;
	@mkdir $(PKGDIR)/$(BIN_NAME)/usr/bin;
	@mkdir $(PKGDIR)/$(BIN_NAME)/etc;
	@mkdir $(PKGDIR)/$(BIN_NAME)/etc/init.d;
	@mkdir $(PKGDIR)/$(BIN_NAME)/etc/$(BIN_NAME);
	@mkdir $(PKGDIR)/$(BIN_NAME)/etc/logrotate.d;
	@mkdir $(PKGDIR)/$(BIN_NAME)/var;
	@mkdir $(PKGDIR)/$(BIN_NAME)/var/log;
	@mkdir $(PKGDIR)/$(BIN_NAME)/var/log/$(BIN_NAME);

package: package_prepare $(BINDIR)/$(BIN_NAME)
	@cp ./pkg_control $(PKGDIR)/$(BIN_NAME)/DEBIAN/control
	@cp ./pkg_postinst $(PKGDIR)/$(BIN_NAME)/DEBIAN/postinst
	@chmod 755 $(PKGDIR)/$(BIN_NAME)/DEBIAN/postinst
	@cp ./pkg_logrotate $(PKGDIR)/$(BIN_NAME)/etc/logrotate.d/$(BIN_NAME)
	@sed -i "s/_program_/$(BIN_NAME)/g" $(PKGDIR)/$(BIN_NAME)/etc/logrotate.d/$(BIN_NAME)
	@sed -i "s/_program_/$(BIN_NAME)/g" $(PKGDIR)/$(BIN_NAME)/DEBIAN/postinst
	@sed -i "s/_program_/$(BIN_NAME)/g" $(PKGDIR)/$(BIN_NAME)/DEBIAN/control
	@sed -i "s/\(.*Package:\)\(.*\)/\1 $(BIN_NAME)/g" $(PKGDIR)/$(BIN_NAME)/DEBIAN/control
	@sed -i "s/\(.*Version:\)\(.*\)/\1 $(VERSION_MAJOR).$(VERSION_MINOR)/g" $(PKGDIR)/$(BIN_NAME)/DEBIAN/control
	@cp $(BINDIR)/$(BIN_NAME) $(PKGDIR)/$(BIN_NAME)/usr/bin
	@cp ./$(BIN_NAME).sh $(PKGDIR)/$(BIN_NAME)/etc/init.d/$(BIN_NAME)

	@sudo dpkg-deb --build $(PKGDIR)/$(BIN_NAME) $(PKGDIR)/$(BIN_NAME)

clean:
	$(RM) $(OBJDIR) $(BINDIR)/$(OUTPUT_FILENAME)
	
.PHONY: clean package_prepare package
