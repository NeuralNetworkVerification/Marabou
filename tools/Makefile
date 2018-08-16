ROOT_DIR = ..

all: boost.unzipped

boost.unzipped:
	wget https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz
	tar xzvf boost_1_68_0.tar.gz
	cd boost_1_68_0; \
	mkdir installed; \
	./bootstrap.sh --prefix=`pwd`/installed --with-libraries=program_options; \
	./b2 install
	@touch $@

include $(ROOT_DIR)/Rules.mk

#
# Local Variables:
# compile-command: "make -C .. "
# tags-file-name: "../TAGS"
# c-basic-offset: 4
# End:
#
