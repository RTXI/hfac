PLUGIN_NAME = hfac

RTXI_INCLUDES =

HEADERS = hfac.h\

SOURCES = hfac.cpp \
          moc_hfac.cpp\

LIBS = -lrtgen

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile
