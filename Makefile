CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2 -g
CFLAGS  += -Iinclude -pthread -MMD -MP
LDFLAGS += -pthread

BUILD := build

# Modulele comune (tot ce nu contine un main())
CORE := parser headers intercept net build client
CORE_OBJ := $(addprefix $(BUILD)/,$(addsuffix .o,$(CORE)))

# Obiectele necesare testului de interceptare (fara retea)
TEST_OBJ := $(BUILD)/parser.o $(BUILD)/headers.o $(BUILD)/intercept.o \
            $(BUILD)/test_intercept.o

.PHONY: all test clean

all: $(BUILD)/proxy $(BUILD)/test_intercept

$(BUILD)/proxy: $(CORE_OBJ) $(BUILD)/main.o
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD)/test_intercept: $(TEST_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/test_intercept.o: tests/test_intercept.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

# Ruleaza testul interactiv de interceptare
test: $(BUILD)/test_intercept
	./$(BUILD)/test_intercept

clean:
	rm -rf $(BUILD)

-include $(wildcard $(BUILD)/*.d)