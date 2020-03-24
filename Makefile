include common.mk

wt:
	cd $(WT_HOME); \
	$(SHELL) autogen.sh; \
	$(SHELL) configure; \
	$(MAKE)

source:
	$(MAKE) -C $(SRC)