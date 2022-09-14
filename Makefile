print-%:; @echo $($*)

CC = clang++
LD = clang++

# CONFIG_UBSAN = 1
CONFIG_ME = 1

ifdef CCFLAGS_CUSTOM
	CCFLAGS = $(CCFLAGS_CUSTOM)
else
	CCFLAGS =
endif

CCFLAGS += $(shell llvm-config --cppflags)
CCFLAGS += -std=c++20 -stdlib=libc++
CCFLAGS += -O2 -g -Wall -Wextra -Wpedantic
CCFLAGS += -Wno-vla-extension
CCFLAGS += -Wno-c99-extensions
CCFLAGS += -Wno-unused-parameter
CCFLAGS += -Wno-gnu-zero-variadic-macro-arguments
CCFLAGS += -Wno-gnu-anonymous-struct
CCFLAGS += -iquoteplugin
CCFLAGS += -Iinclude
CCFLAGS += -Ilib/fmt/include
CCFLAGS += -Ilib/nameof/include

LDFLAGS = -lstdc++
LDFLAGS += $(shell llvm-config --ldflags --libs)
LDFLAGS += $(shell llvm-config --libdir)/libclang-cpp.dylib

ifdef CONFIG_UBSAN
CCFLAGS += -fsanitize=undefined,address -fno-omit-frame-pointer
LDFLAGS += -fsanitize=undefined,address
endif

# me-specific
ifdef CONFIG_ME
	CC = $(shell brew --prefix llvm)/bin/clang++
	LD = $(CC)
endif

# use ccache if present
ifeq (,$(shell command -v ccache 2> /dev/null))
	CCACHE =
else
	CCACHE = #ccache
endif

BIN = bin

PLUGIN_SRC = $(shell find plugin -name "*.cpp")
PLUGIN_OBJ = $(PLUGIN_SRC:.cpp=.o)
PLUGIN_DEP = $(PLUGIN_OBJ:.o=.d)
PLUGIN = $(BIN)/libarchimedes-plugin.dylib

LIB_SRC = $(shell find runtime -name "*.cpp")
LIB_OBJ = $(LIB_SRC:.cpp=.o)
LIB_DEP = $(LIB_OBJ:.o=.d)

COMMON_SRC = $(shell find common -name "*.cpp")
COMMON_OBJ = $(COMMON_SRC:.cpp=.o)
COMMON_DEP = $(COMMON_OBJ:.o=.d)

SHARED = $(BIN)/libarchimedes.dylib
STATIC = $(BIN)/libarchimedes.a

TEST_DIR = test

TEST_SRC 			= $(shell find $(TEST_DIR) -name "*.test.cpp")
TEST_DEP 			= $(TEST_SRC:.cpp=.d)
TEST_OBJ 			= $(TEST_SRC:.cpp=.o)
TEST_TYPES_OBJ 		= $(TEST_SRC:.cpp=.types.o)
TEST_OUT 			= $(TEST_SRC:.test.cpp=)
TEST_OUT_NAMES 		= $(notdir $(TEST_OUT))

TEST_RUNNER_SRC = $(TEST_DIR)/tests.cpp
TEST_RUNNER_DEP = $(TEST_RUNNER_SRC:.cpp=.d)
TEST_RUNNER_OUT = $(TEST_RUNNER_SRC:.cpp=)

all: dirs plugin shared static

$(BIN):
	mkdir -p $@

dirs: $(BIN)

-include $(PLUGIN_DEP) $(LIB_DEP) $(TEST_DEP) $(TEST_RUNNER_DEP)

%.test.o %.test.types.o: %.test.cpp %.test.hpp $(PLUGIN)
	$(CCACHE) $(CC) -o $@ -MMD -c $(CCFLAGS) 			      	\
		-fplugin=$(PLUGIN)					                  	\
		-fplugin-arg-archimedes-exclude-ns-std 			      	\
		-fplugin-arg-archimedes-header-include/archimedes.hpp 	\
		-fplugin-arg-archimedes-file-$< 			          	\
		-fplugin-arg-archimedes-file-$(<:.cpp=.hpp)           	\
		-fplugin-arg-archimedes-out-$(<:.cpp=.types.o) 	  		\
		$<

$(TEST_OUT): %: %.test.o %.test.types.o $(STATIC)
	$(LD) -o $@ $(filter %.o,$^) -Lbin -larchimedes $(LDFLAGS)

$(TEST_RUNNER_OUT): %: %.cpp
	$(CCACHE) $(CC) -o $@ $(CCFLAGS) $<

test: $(TEST_RUNNER_OUT) $(TEST_OUT) FORCE
	./$(TEST_RUNNER_OUT) $(TEST_DIR)

test-%: $(TEST_DIR)/% $(TEST_RUNNER_OUT)
	./$(TEST_RUNNER_OUT) $(TEST_DIR) $(notdir $<)

test-source-%: test/%.test.cpp test/%.test.hpp $(PLUGIN)
	$(CCACHE) $(CC) -o $(<:.cpp=.o) -MMD -c $(CCFLAGS) 			\
		-fplugin=$(PLUGIN)					                  	\
		-fplugin-arg-archimedes-exclude-ns-std 			      	\
		-fplugin-arg-archimedes-header-include/archimedes.hpp 	\
		-fplugin-arg-archimedes-file-$< 			          	\
		-fplugin-arg-archimedes-file-$(<:.cpp=.hpp)           	\
		-fplugin-arg-archimedes-out-$(<:.cpp=.types.cpp) 	  	\
		-fplugin-arg-archimedes-emit-source 					\
		-fplugin-arg-archimedes-emit-verbose 					\
		-fplugin-arg-archimedes-verbose 						\
		$<

FORCE:

$(PLUGIN): $(PLUGIN_OBJ) $(COMMON_OBJ) | dirs
	$(LD) -g -shared -o $(PLUGIN) $(filter %.o,$^) $(LDFLAGS)

plugin: $(PLUGIN)

$(SHARED): $(LIB_OBJ) $(COMMON_OBJ) | dirs
	$(LD) -g -shared -o $(SHARED) $(filter %.o,$^) $(LDFLAGS)

shared: $(SHARED)

$(STATIC): $(LIB_OBJ) $(COMMON_OBJ) | dirs
	ar rcs $(STATIC) $(filter %.o,$^)

static: $(STATIC)

deps: $(DEPS) $(PCHDEP) $(SHADERS_DEP)

$(LIB_OBJ) $(PLUGIN_OBJ) $(COMMON_OBJ): %.o: %.cpp
	$(CCACHE) $(CC) -o $@ -MMD -c $(CCFLAGS) $<

clean:
	cd test && find . -name "*.d" -delete
	cd test && find . -type f ! -name '*.*' -delete
	cd runtime && find . -name "*.d" -delete
	cd plugin && find . -name "*.d" -delete
	find . -name "*.o" -delete
	find . -name "*.types.cpp" -delete
	rm -rf $(BIN)

time: dirs
	make clean && make -j 10 plugin static && time make -j 10 $(TEST_OUT)
