#
# Places
#

PROJECT_DIR       = $(ROOT_DIR)
SRC_DIR           = $(ROOT_DIR)/src

TOOLS_DIR	  = $(PROJECT_DIR)/tools
CXXTEST_DIR 	  = $(TOOLS_DIR)/cxxtest
BOOST_LIBS_DIR	  = $(TOOLS_DIR)/boost_1_68_0/installed/lib
BOOST_INCLUDES	  = $(TOOLS_DIR)/boost_1_68_0/installed/include
GUROBI_LIBS 	  = $(TOOLS_DIR)/gurobi811/lib
GUROBI_INCLUDES	  = $(TOOLS_DIR)/gurobi811/include

INPUT_PARSER_DIR  = $(SRC_DIR)/input_parsers

COMMON_DIR 	  = $(SRC_DIR)/common
COMMON_MOCK_DIR   = $(COMMON_DIR)/mock
COMMON_REAL_DIR   = $(COMMON_DIR)/real
COMMON_TEST_DIR   = $(COMMON_DIR)/tests

ENGINE_DIR        = $(SRC_DIR)/engine
ENGINE_MOCK_DIR   = $(ENGINE_DIR)/mock
ENGINE_REAL_DIR   = $(ENGINE_DIR)/real
ENGINE_TEST_DIR   = $(ENGINE_DIR)/tests

CONFIGURATION_DIR = $(SRC_DIR)/configuration

BASIS_FACTORIZATION_DIR = $(SRC_DIR)/basis_factorization

REGRESS_DIR	  = $(PROJECT_DIR)/regress

#
# Local Variables:
# compile-command: "make -C . "
# tags-file-name: "./TAGS"
# c-basic-offset: 4
# End:
#
