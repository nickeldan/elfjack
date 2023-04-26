APPS := $(patsubst $(APP_DIR)/%.c,%,$(wildcard $(APP_DIR)/*.c))

%: $(APP_DIR)/%.c $(EJ_STATIC_LIBRARY)
	$(CC) $(CFLAGS) $(EJ_INCLUDE_FLAGS) $^ -o $@

app_clean:
	@rm -f $(APPS)

CLEAN_TARGETS += app_clean
