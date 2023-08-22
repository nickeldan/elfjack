CFLAGS := -std=gnu99 -fdiagnostics-color -Wall -Wextra -fvisibility=hidden
ifeq ($(debug),yes)
    CFLAGS += -O0 -g -DDEBUG
else
    CFLAGS += -O2 -DNDEBUG
endif

BUILD_DEPS :=
ifeq ($(MAKECMDGOALS),clean)
else ifeq ($(MAKECMDGOALS),format)
else ifeq ($(MAKECMDGOALS),uninstall)
else
    BUILD_DEPS := yes
endif

all: _all

EJ_DIR := .
include make.mk

APP_DIR := apps
include $(APP_DIR)/make.mk

.PHONY: all _all format install uninstall clean

_all: $(EJ_SHARED_LIBRARY) $(EJ_STATIC_LIBRARY) $(APPS)

format:
	find . -name '*.[hc]' -print0 | xargs -0 -n 1 clang-format -i

install: /usr/local/lib/$(notdir $(EJ_SHARED_LIBRARY)) $(foreach file,$(EJ_HEADER_FILES),/usr/local/include/elfjack/$(notdir $(file)))
	ldconfig

/usr/local/lib/$(notdir $(EJ_SHARED_LIBRARY)): $(EJ_SHARED_LIBRARY)
	cp $< $@

/usr/local/include/elfjack/%.h: include/elfjack/%.h
	@mkdir -p $(@D)
	cp $< $@

uninstall:
	rm -rf /usr/local/include/elfjack
	rm -f /usr/local/lib/$(notdir $(EJ_SHARED_LIBRARY))
	ldconfig

clean: $(CLEAN_TARGETS)
	@rm -f $(DEPS_FILES)
