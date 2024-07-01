# Normally this makefile shouldn't be called directly and we expect the output
# path to be on a certain location to fit together with the other OP-TEE
# gits and helper scripts.

include ../../scripts/common.mk
out-dir := $(call strip-trailing-slashes-and-dots,$(O))
ifeq ($(out-dir),)
$(error invalid output directory (O=$(O)))
endif

include $(TA_DEV_KIT_DIR)/mk/conf.mk

# Prevent use of LDFLAGS from the environment. For example, yocto exports
# LDFLAGS that are suitable for the client applications, not for TAs
LDFLAGS=

CTA_DIRS :=

.PHONY: all
all: ta

.PHONY: ta
ta: 
	$(q)$(foreach dir,$(CTA_DIRS), $(MAKE) -C $(dir) O=$(out-dir)/ta/$(dir) &&) true

# remove build directories including ta/<ta-name>/ directories.
# Note: $(out-dir) may be a relative path.
RMDIR := rmdir --ignore-fail-on-non-empty
define rm-build-subdir
echo `[ -d $1 ] && { cd $1; $(RMDIR) $(out-dir)/ta $(out-dir) 2> /dev/null; true; }` > /dev/null
endef

.PHONY: clean
clean:
	$(q)$(foreach dir,$(CTA_DIRS), [ ! -d $(dir) ] || $(MAKE) -C $(dir) O=$(out-dir)/ta/$(dir) $@ &&) true
	$(q)$(foreach dir,$(CTA_DIRS), $(call rm-build-subdir,$(dir));)
