ROOT_DIR = ..

all: boost.unzipped

boost.unzipped:
	@echo Downloading Boost...
	wget https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz
	@echo Extracting Boost...
	@tar xzvf boost_1_68_0.tar.gz >> /dev/null
	@echo Compiling Boost...
	cd boost_1_68_0; \
	mkdir installed; \
	./bootstrap.sh --prefix=`pwd`/installed --with-libraries=program_options >> /dev/null ; \
	./b2 install >> /dev/null
	@touch $@

include $(ROOT_DIR)/Rules.mk

#
# Local Variables:
# compile-command: "make -C .. "
# tags-file-name: "../TAGS"
# c-basic-offset: 4
# End:
#
