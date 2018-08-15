SOURCES += \
	GlobalConfiguration.cpp \
	OptionParser.cpp \
	Options.cpp \

LIBRARY_DIR += \
	$(BOOST_LIBS) \

LIBRARIES += \
	:libboost_program_options.a \

LOCAL_INCLUDES += \
	$(BOOST_INCLUDES)

#
# Local Variables:
# compile-command: "make -C ../.. "
# tags-file-name: "../../TAGS"
# c-basic-offset: 4
# End:
#
