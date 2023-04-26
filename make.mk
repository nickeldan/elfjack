ifndef EJ_MK
EJ_MK :=

EJ_LIB_DIR ?= $(EJ_DIR)
EJ_OBJ_DIR ?= $(EJ_DIR)/src

EJ_SHARED_LIBRARY := $(EJ_LIB_DIR)/libelfjack.so
EJ_STATIC_LIBRARY := $(EJ_LIB_DIR)/libelfjack.a

EJ_SOURCE_FILES := $(wildcard $(EJ_DIR)/src/*.c)
EJ_OBJECT_FILES := $(patsubst $(EJ_DIR)/src/%.c,$(EJ_OBJ_DIR)/%.o,$(EJ_SOURCE_FILES))
EJ_HEADER_FILES := $(wildcard $(EJ_DIR)/include/elfjack/*.h)
EJ_INCLUDE_FLAGS := -I$(EJ_DIR)/include

EJ_DEPS_FILE := $(EJ_OBJ_DIR)/deps.mk
DEPS_FILES += $(EJ_DEPS_FILE)

BUILD_DEPS ?= $(if $(MAKECMDGOALS),$(subst clean,,$(MAKECMDGOALS)),yes)

ifneq ($(BUILD_DEPS),)

$(EJ_DEPS_FILE): $(EJ_SOURCE_FILES) $(EJ_HEADER_FILES)
	@mkdir -p $(@D)
	@rm -f $@
	for file in $(EJ_SOURCE_FILES); do \
	    echo "$(EJ_OBJ_DIR)/`$(CC) $(EJ_INCLUDE_FLAGS) -MM $$file`" >> $@ && \
	    echo '\t$$(CC) $$(CFLAGS) -fpic -ffunction-sections $(EJ_INCLUDE_FLAGS) -c $$< -o $$@' >> $@; \
	done
include $(EJ_DEPS_FILE)

endif

$(EJ_SHARED_LIBRARY): $(EJ_OBJECT_FILES)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -shared -o $@ $^

$(EJ_STATIC_LIBRARY): $(EJ_OBJECT_FILES)
	@mkdir -p $(@D)
	$(AR) rcs $@ $^

ej_clean:
	@rm -f $(EJ_SHARED_LIBRARY) $(EJ_STATIC_LIBRARY) $(EJ_OBJECT_FILES)

CLEAN_TARGETS += ej_clean

endif
