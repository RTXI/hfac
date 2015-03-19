PLUGIN_NAME = hfac

RTXI_INCLUDES = /usr/local/lib/rtxi_includes

HEADERS = hfac.h\
          $(RTXI_INCLUDES)/gen_sine.h\
          $(RTXI_INCLUDES)/generator.h

SOURCES = hfac.cpp \
          moc_hfac.cpp\
          $(RTXI_INCLUDES)/gen_sine.cpp\
          $(RTXI_INCLUDES)/generator.cpp

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile
