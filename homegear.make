# GNU Make project makefile autogenerated by Premake
ifndef config
  config=release
endif

ifndef verbose
  SILENT = @
endif

ifndef CC
  CC = gcc
endif

ifndef CXX
  CXX = g++
endif

ifndef AR
  AR = ar
endif

ifndef RESCOMP
  ifdef WINDRES
    RESCOMP = $(WINDRES)
  else
    RESCOMP = windres
  endif
endif

ifeq ($(config),release)
  OBJDIR     = obj/Release/homegear
  TARGETDIR  = bin/Release
  TARGET     = $(TARGETDIR)/homegear
  DEFINES   += -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED -DHAVE_SSIZE_T=1 -DSCRIPTENGINE -DEVENTHANDLER -DOPENSSL -DSPIINTERFACES -DNDEBUG
  INCLUDES  += 
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -O2 -Wall -std=c++11
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -Llib/Release -s -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-Bstatic -lrpc -Wl,-Bdynamic -ldl -lpthread -lreadline -lgcrypt -lgnutls -luser -lcli -levents -lgd -lupnp -lmqtt -ldatabase -lscriptengine -lbase -lgpg-error -lsqlite3 -lpaho.mqtt.c -lcrypto -lssl -lphp5 -Wl,--as-needed
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LIBS      += lib/Release/librpc.a lib/Release/libuser.a lib/Release/libcli.a lib/Release/libevents.a lib/Release/libgd.a lib/Release/libupnp.a lib/Release/libmqtt.a lib/Release/libdatabase.a lib/Release/libscriptengine.a lib/Release/libbase.a lib/Release/libpaho.mqtt.c.a
  LDDEPS    += lib/Release/librpc.a lib/Release/libuser.a lib/Release/libcli.a lib/Release/libevents.a lib/Release/libgd.a lib/Release/libupnp.a lib/Release/libmqtt.a lib/Release/libdatabase.a lib/Release/libscriptengine.a lib/Release/libbase.a lib/Release/libpaho.mqtt.c.a
  LINKCMD    = $(CXX) -o $(TARGET) $(OBJECTS) $(RESOURCES) $(ARCH) $(LIBS) $(LDFLAGS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

ifeq ($(config),debug)
  OBJDIR     = obj/Debug/homegear
  TARGETDIR  = bin/Debug
  TARGET     = $(TARGETDIR)/homegear
  DEFINES   += -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED -DHAVE_SSIZE_T=1 -DSCRIPTENGINE -DEVENTHANDLER -DOPENSSL -DSPIINTERFACES -DDEBUG
  INCLUDES  += 
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -g -Wall -std=c++11
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -Llib/Debug -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-Bstatic -lrpc -Wl,-Bdynamic -ldl -lpthread -lreadline -lgcrypt -lgnutls -luser -lcli -levents -lgd -lupnp -lmqtt -ldatabase -lscriptengine -lbase -lgpg-error -lsqlite3 -lpaho.mqtt.c -lcrypto -lssl -lphp5 -Wl,--as-needed
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LIBS      += lib/Debug/librpc.a lib/Debug/libuser.a lib/Debug/libcli.a lib/Debug/libevents.a lib/Debug/libgd.a lib/Debug/libupnp.a lib/Debug/libmqtt.a lib/Debug/libdatabase.a lib/Debug/libscriptengine.a lib/Debug/libbase.a lib/Debug/libpaho.mqtt.c.a
  LDDEPS    += lib/Debug/librpc.a lib/Debug/libuser.a lib/Debug/libcli.a lib/Debug/libevents.a lib/Debug/libgd.a lib/Debug/libupnp.a lib/Debug/libmqtt.a lib/Debug/libdatabase.a lib/Debug/libscriptengine.a lib/Debug/libbase.a lib/Debug/libpaho.mqtt.c.a
  LINKCMD    = $(CXX) -o $(TARGET) $(OBJECTS) $(RESOURCES) $(ARCH) $(LIBS) $(LDFLAGS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

ifeq ($(config),profiling)
  OBJDIR     = obj/Profiling/homegear
  TARGETDIR  = bin/Profiling
  TARGET     = $(TARGETDIR)/homegear
  DEFINES   += -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED -DHAVE_SSIZE_T=1 -DSCRIPTENGINE -DEVENTHANDLER -DOPENSSL -DSPIINTERFACES -DNDEBUG
  INCLUDES  += 
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -O2 -g -Wall -std=c++11 -pg
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -Llib/Profiling -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-Bstatic -lrpc -Wl,-Bdynamic -ldl -lpthread -lreadline -lgcrypt -lgnutls -luser -lcli -levents -lgd -lupnp -lmqtt -ldatabase -lscriptengine -lbase -lgpg-error -lsqlite3 -lpaho.mqtt.c -lcrypto -lssl -lphp5 -Wl,--as-needed -pg
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LIBS      += lib/Profiling/librpc.a lib/Profiling/libuser.a lib/Profiling/libcli.a lib/Profiling/libevents.a lib/Profiling/libgd.a lib/Profiling/libupnp.a lib/Profiling/libmqtt.a lib/Profiling/libdatabase.a lib/Profiling/libscriptengine.a lib/Profiling/libbase.a lib/Profiling/libpaho.mqtt.c.a
  LDDEPS    += lib/Profiling/librpc.a lib/Profiling/libuser.a lib/Profiling/libcli.a lib/Profiling/libevents.a lib/Profiling/libgd.a lib/Profiling/libupnp.a lib/Profiling/libmqtt.a lib/Profiling/libdatabase.a lib/Profiling/libscriptengine.a lib/Profiling/libbase.a lib/Profiling/libpaho.mqtt.c.a
  LINKCMD    = $(CXX) -o $(TARGET) $(OBJECTS) $(RESOURCES) $(ARCH) $(LIBS) $(LDFLAGS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

OBJECTS := \
	$(OBJDIR)/main.o \
	$(OBJDIR)/PhysicalInterfaces.o \
	$(OBJDIR)/DatabaseController.o \
	$(OBJDIR)/FamilyController.o \

RESOURCES := \

SHELLTYPE := msdos
ifeq (,$(ComSpec)$(COMSPEC))
  SHELLTYPE := posix
endif
ifeq (/bin,$(findstring /bin,$(SHELL)))
  SHELLTYPE := posix
endif

.PHONY: clean prebuild prelink

all: $(TARGETDIR) $(OBJDIR) prebuild prelink $(TARGET)
	@:

$(TARGET): $(GCH) $(OBJECTS) $(LDDEPS) $(RESOURCES)
	@echo Linking homegear
	$(SILENT) $(LINKCMD)
	$(POSTBUILDCMDS)

$(TARGETDIR):
	@echo Creating $(TARGETDIR)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(TARGETDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(TARGETDIR))
endif

$(OBJDIR):
	@echo Creating $(OBJDIR)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(OBJDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(OBJDIR))
endif

clean:
	@echo Cleaning homegear
ifeq (posix,$(SHELLTYPE))
	$(SILENT) rm -f  $(TARGET)
	$(SILENT) rm -rf $(OBJDIR)
else
	$(SILENT) if exist $(subst /,\\,$(TARGET)) del $(subst /,\\,$(TARGET))
	$(SILENT) if exist $(subst /,\\,$(OBJDIR)) rmdir /s /q $(subst /,\\,$(OBJDIR))
endif

prebuild:
	$(PREBUILDCMDS)

prelink:
	$(PRELINKCMDS)

ifneq (,$(PCH))
$(GCH): $(PCH)
	@echo $(notdir $<)
ifeq (posix,$(SHELLTYPE))
	-$(SILENT) cp $< $(OBJDIR)
else
	$(SILENT) xcopy /D /Y /Q "$(subst /,\,$<)" "$(subst /,\,$(OBJDIR))" 1>nul
endif
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
endif

$(OBJDIR)/main.o: main.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/PhysicalInterfaces.o: Libraries/Systems/PhysicalInterfaces.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/DatabaseController.o: Libraries/Systems/DatabaseController.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/FamilyController.o: Libraries/Systems/FamilyController.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"

-include $(OBJECTS:%.o=%.d)
