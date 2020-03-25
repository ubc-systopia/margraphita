include common.mk

wt:
	cd $(WT_HOME) && \
	./autogen.sh && \
	./configure && \
	$(MAKE)

source:
	$(MAKE) -C $(SRC)
