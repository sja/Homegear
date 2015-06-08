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
  OBJDIR     = obj/Release/base
  TARGETDIR  = lib/Release
  TARGET     = $(TARGETDIR)/libbase.a
  DEFINES   += -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED -DHAVE_SSIZE_T=1 -DSCRIPTENGINE -DEVENTHANDLER -DOPENSSL -DSPIINTERFACES -D_GLIBCXX_USE_C99 -D_GLIBCXX_USE_C99_MATH -D_GLIBCXX_USE_C99_MATH_TR1 -D_WITH_DPRINTF -DNDEBUG
  INCLUDES  += 
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -O2 -Wall -std=c++11 -fPIC
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -s -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LIBS      += 
  LDDEPS    += 
  LINKCMD    = $(AR) -rcs $(TARGET) $(OBJECTS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

ifeq ($(config),debug)
  OBJDIR     = obj/Debug/base
  TARGETDIR  = lib/Debug
  TARGET     = $(TARGETDIR)/libbase.a
  DEFINES   += -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED -DHAVE_SSIZE_T=1 -DSCRIPTENGINE -DEVENTHANDLER -DOPENSSL -DSPIINTERFACES -D_GLIBCXX_USE_C99 -D_GLIBCXX_USE_C99_MATH -D_GLIBCXX_USE_C99_MATH_TR1 -D_WITH_DPRINTF -DDEBUG
  INCLUDES  += 
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -g -Wall -std=c++11 -fPIC
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LIBS      += 
  LDDEPS    += 
  LINKCMD    = $(AR) -rcs $(TARGET) $(OBJECTS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

ifeq ($(config),profiling)
  OBJDIR     = obj/Profiling/base
  TARGETDIR  = lib/Profiling
  TARGET     = $(TARGETDIR)/libbase.a
  DEFINES   += -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED -DHAVE_SSIZE_T=1 -DSCRIPTENGINE -DEVENTHANDLER -DOPENSSL -DSPIINTERFACES -D_GLIBCXX_USE_C99 -D_GLIBCXX_USE_C99_MATH -D_GLIBCXX_USE_C99_MATH_TR1 -D_WITH_DPRINTF -DNDEBUG
  INCLUDES  += 
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -O2 -g -Wall -std=c++11 -fPIC -pg
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -pg
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LIBS      += 
  LDDEPS    += 
  LINKCMD    = $(AR) -rcs $(TARGET) $(OBJECTS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

ifeq ($(config),release_bsd)
  CC         = gcc
  CXX        = g++
  OBJDIR     = obj/bsd/Release/base
  TARGETDIR  = lib/Release
  TARGET     = $(TARGETDIR)/libbase.a
  DEFINES   += -D_GLIBCXX_USE_C99 -D_GLIBCXX_USE_C99_MATH -D_GLIBCXX_USE_C99_MATH_TR1 -D_WITH_DPRINTF -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED -DHAVE_SSIZE_T=1 -DSCRIPTENGINE -DEVENTHANDLER -DOPENSSL -DNDEBUG
  INCLUDES  += 
  CPPFLAGS  += -MMD -D_GLIBCXX_USE_NANOSLEEP -D_FORTIFY_SOURCE=2 -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -O2 -Wall -std=c++11 -fPIC
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -s -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LIBS      += 
  LDDEPS    += 
  LINKCMD    = $(AR) -rcs $(TARGET) $(OBJECTS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

ifeq ($(config),debug_bsd)
  CC         = gcc
  CXX        = g++
  OBJDIR     = obj/bsd/Debug/base
  TARGETDIR  = lib/Debug
  TARGET     = $(TARGETDIR)/libbase.a
  DEFINES   += -D_GLIBCXX_USE_C99 -D_GLIBCXX_USE_C99_MATH -D_GLIBCXX_USE_C99_MATH_TR1 -D_WITH_DPRINTF -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED -DHAVE_SSIZE_T=1 -DSCRIPTENGINE -DEVENTHANDLER -DOPENSSL -DDEBUG
  INCLUDES  += 
  CPPFLAGS  += -MMD -D_GLIBCXX_USE_NANOSLEEP -D_FORTIFY_SOURCE=2 -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -g -Wall -std=c++11 -fPIC
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LIBS      += 
  LDDEPS    += 
  LINKCMD    = $(AR) -rcs $(TARGET) $(OBJECTS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

ifeq ($(config),profiling_bsd)
  CC         = gcc
  CXX        = g++
  OBJDIR     = obj/bsd/Profiling/base
  TARGETDIR  = lib/Profiling
  TARGET     = $(TARGETDIR)/libbase.a
  DEFINES   += -D_GLIBCXX_USE_C99 -D_GLIBCXX_USE_C99_MATH -D_GLIBCXX_USE_C99_MATH_TR1 -D_WITH_DPRINTF -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED -DHAVE_SSIZE_T=1 -DSCRIPTENGINE -DEVENTHANDLER -DOPENSSL -DNDEBUG
  INCLUDES  += 
  CPPFLAGS  += -MMD -D_GLIBCXX_USE_NANOSLEEP -D_FORTIFY_SOURCE=2 -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -O2 -g -Wall -std=c++11 -fPIC -pg
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -pg
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LIBS      += 
  LDDEPS    += 
  LINKCMD    = $(AR) -rcs $(TARGET) $(OBJECTS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

OBJECTS := \
	$(OBJDIR)/BaseLib.o \
	$(OBJDIR)/IEvents.o \
	$(OBJDIR)/Crypt.o \
	$(OBJDIR)/Math.o \
	$(OBJDIR)/Color.o \
	$(OBJDIR)/HelperFunctions.o \
	$(OBJDIR)/Base64.o \
	$(OBJDIR)/Output.o \
	$(OBJDIR)/PhysicalParameter.o \
	$(OBJDIR)/Variable.o \
	$(OBJDIR)/Device.o \
	$(OBJDIR)/LogicalParameter.o \
	$(OBJDIR)/Devices.o \
	$(OBJDIR)/Peer.o \
	$(OBJDIR)/Central.o \
	$(OBJDIR)/IPhysicalInterface.o \
	$(OBJDIR)/DeviceTypes.o \
	$(OBJDIR)/LogicalDevice.o \
	$(OBJDIR)/Packet.o \
	$(OBJDIR)/DeviceFamily.o \
	$(OBJDIR)/UpdateInfo.o \
	$(OBJDIR)/ServiceMessages.o \
	$(OBJDIR)/FileDescriptorManager.o \
	$(OBJDIR)/SerialDeviceManager.o \
	$(OBJDIR)/BinaryEncoder.o \
	$(OBJDIR)/XMLRPCDecoder.o \
	$(OBJDIR)/BinaryDecoder.o \
	$(OBJDIR)/HTTP.o \
	$(OBJDIR)/JsonEncoder.o \
	$(OBJDIR)/RPCHeader.o \
	$(OBJDIR)/RPCDecoder.o \
	$(OBJDIR)/JsonDecoder.o \
	$(OBJDIR)/XMLRPCEncoder.o \
	$(OBJDIR)/RPCEncoder.o \
	$(OBJDIR)/WebSocket.o \
	$(OBJDIR)/SocketOperations.o \
	$(OBJDIR)/SerialReaderWriter.o \
	$(OBJDIR)/HTTPClient.o \
	$(OBJDIR)/Threads.o \
	$(OBJDIR)/Settings.o \

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
	@echo Linking base
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
	@echo Cleaning base
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

$(OBJDIR)/BaseLib.o: Modules/Base/BaseLib.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/IEvents.o: Modules/Base/IEvents.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/Crypt.o: Modules/Base/HelperFunctions/Crypt.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/Math.o: Modules/Base/HelperFunctions/Math.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/Color.o: Modules/Base/HelperFunctions/Color.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/HelperFunctions.o: Modules/Base/HelperFunctions/HelperFunctions.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/Base64.o: Modules/Base/HelperFunctions/Base64.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/Output.o: Modules/Base/Output/Output.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/PhysicalParameter.o: Modules/Base/RPC/PhysicalParameter.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/Variable.o: Modules/Base/RPC/Variable.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/Device.o: Modules/Base/RPC/Device.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/LogicalParameter.o: Modules/Base/RPC/LogicalParameter.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/Devices.o: Modules/Base/RPC/Devices.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/Peer.o: Modules/Base/Systems/Peer.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/Central.o: Modules/Base/Systems/Central.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/IPhysicalInterface.o: Modules/Base/Systems/IPhysicalInterface.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/DeviceTypes.o: Modules/Base/Systems/DeviceTypes.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/LogicalDevice.o: Modules/Base/Systems/LogicalDevice.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/Packet.o: Modules/Base/Systems/Packet.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/DeviceFamily.o: Modules/Base/Systems/DeviceFamily.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/UpdateInfo.o: Modules/Base/Systems/UpdateInfo.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/ServiceMessages.o: Modules/Base/Systems/ServiceMessages.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/FileDescriptorManager.o: Modules/Base/Managers/FileDescriptorManager.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/SerialDeviceManager.o: Modules/Base/Managers/SerialDeviceManager.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/BinaryEncoder.o: Modules/Base/Encoding/BinaryEncoder.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/XMLRPCDecoder.o: Modules/Base/Encoding/XMLRPCDecoder.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/BinaryDecoder.o: Modules/Base/Encoding/BinaryDecoder.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/HTTP.o: Modules/Base/Encoding/HTTP.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/JsonEncoder.o: Modules/Base/Encoding/JsonEncoder.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/RPCHeader.o: Modules/Base/Encoding/RPCHeader.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/RPCDecoder.o: Modules/Base/Encoding/RPCDecoder.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/JsonDecoder.o: Modules/Base/Encoding/JsonDecoder.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/XMLRPCEncoder.o: Modules/Base/Encoding/XMLRPCEncoder.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/RPCEncoder.o: Modules/Base/Encoding/RPCEncoder.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/WebSocket.o: Modules/Base/Encoding/WebSocket.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/SocketOperations.o: Modules/Base/Sockets/SocketOperations.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/SerialReaderWriter.o: Modules/Base/Sockets/SerialReaderWriter.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/HTTPClient.o: Modules/Base/Sockets/HTTPClient.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/Threads.o: Modules/Base/Threads/Threads.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/Settings.o: Modules/Base/Settings/Settings.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"

-include $(OBJECTS:%.o=%.d)
