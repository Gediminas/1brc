CCG = gcc
CCC = clang

CFLAGS = -march=skylake -march=native -mtune=native -funroll-loops -Wall -Wextra -Isrc 
# CFLAGS = -march=native -mtune=native -Wall -Wextra -Isrc 

LIBS = -luring

RELEASE_FLAGS       = -Ofast -DNDEBUG -DRELEASE
RELEASE_SAFE_FLAGS  = -Ofast -DNDEBUG -DRELEASE -DSAFE
RELEASE_DEBUG_FLAGS = -Ofast -g -DNDEBUG -DRELEASE
DEBUG_FLAGS         = -g -DDEBUG -DSAFE

APP = 1brc
GEN = mappings_gen
SRC = src
BIN = bin
DEP := bin/deps

DEP_FLAGS = -MT $@ -MMD -MP -MF $(DEP)/$*.d

all:               | $(BIN) $(DEP) debug release release-safe release-debug debug-gcc release-gcc release-debug-gcc gen gen-debug

release:           | $(BIN) $(DEP) $(APP:%=$(BIN)/%)
release-safe:      | $(BIN) $(DEP) $(APP:%=$(BIN)/%_safe)
release-debug:     | $(BIN) $(DEP) $(APP:%=$(BIN)/%_rdebug)
debug:             | $(BIN) $(DEP) $(APP:%=$(BIN)/%_debug)

release-gcc:       | $(BIN) $(DEP) $(APP:%=$(BIN)/%_gcc)
release-debug-gcc: | $(BIN) $(DEP) $(APP:%=$(BIN)/%_rdebug_gcc)
debug-gcc:         | $(BIN) $(DEP) $(APP:%=$(BIN)/%_debug_gcc)

gen:               | $(BIN) $(DEP) $(GEN:%=$(BIN)/%)
gen-debug:         | $(BIN) $(DEP) $(GEN:%=$(BIN)/%_debug)

$(BIN):
	mkdir -p $(BIN)

$(DEP):
	mkdir -p $(DEP)

clean:
	rm -rf $(BIN)/*

###########################
# CLANG

$(BIN)/%: $(SRC)/%.c
	$(CCC) $< -o $@ $(CFLAGS) $(RELEASE_FLAGS) $(DEP_FLAGS) $(LIBS)

$(BIN)/%_safe: $(SRC)/%.c
	$(CCC) $< -o $@ $(CFLAGS) $(RELEASE_SAFE_FLAGS) $(DEP_FLAGS) $(LIBS)

$(BIN)/%_rdebug: $(SRC)/%.c
	$(CCC) $< -o $@ $(CFLAGS) $(RELEASE_DEBUG_FLAGS) $(DEP_FLAGS) $(LIBS)

$(BIN)/%_debug: $(SRC)/%.c
	$(CCC) $< -o $@ $(CFLAGS) $(DEBUG_FLAGS) $(DEP_FLAGS) $(LIBS)


###########################
# GCC

$(BIN)/%_gcc: $(SRC)/%.c
	$(CCG) $< -o $@ $(CFLAGS) $(RELEASE_FLAGS) $(DEP_FLAGS) $(LIBS)

$(BIN)/%_safe_gcc: $(SRC)/%.c
	$(CCG) $< -o $@ $(CFLAGS) $(RELEASE_SAFE_FLAGS) $(DEP_FLAGS) $(LIBS)

$(BIN)/%_rdebug_gcc: $(SRC)/%.c
	$(CCG) $< -o $@ $(CFLAGS) $(RELEASE_DEBUG_FLAGS) $(DEP_FLAGS) $(LIBS)

$(BIN)/%_debug_gcc: $(SRC)/%.c
	$(CCG) $< -o $@ $(CFLAGS) $(DEBUG_FLAGS) $(DEP_FLAGS) $(LIBS)


###########################

-include $(DEP)/*.d

.PHONY: all clean debug release release-debug debug-gcc release-gcc release-debug-gcc gen gen-debug
