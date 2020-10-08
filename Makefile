#
# Top level Makefile
#

include common.mk

TARGETS := src


source:
	$(MAKE) -C $(SRC)

wt:
	cd $(WT_HOME) && \
	./autogen.sh && \
	./configure && \
	$(MAKE)


