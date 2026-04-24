# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Variables
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O3 -march=native -Wall -Wextra -Wpedantic -fopenmp -mavx512f -mavx512bw -mavx512dq
LDFLAGS  ?=

TARGETS = burning_oracle turan_oracle evasion_oracle wmat_oracle
SRCS    = src/burning_oracle.cpp src/turan_oracle.cpp src/evasion_oracle.cpp src/wmat_oracle.cpp

.DEFAULT_GOAL := _help

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Print Helpers
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
define print_err
	printf "\033[1;31m%s\033[0m\n" "$(1)"
endef

define print_success
	printf "\033[1;34m✓ %s\033[0m\n" "$(1)"
endef

define print_info
	printf "\033[1;36m%s\033[0m\n" "$(1)"
endef

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Build
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.PHONY: all clean _help help format lint lean vars build run test bundle

all: $(TARGETS) ##H Build all oracles

bundle: clean ##H Bundle project into bundle.zip
	@$(call print_info,Bundling project)
	zip -r bundle.zip . -x ".git/*" ".lake/*" "bundle.zip" "proofs/.lake/*" "*.o"
	@$(call print_success,Bundle created: bundle.zip)

build: all ##H Alias for 'all'

run: build/burning ##H Run the Phase 1 Burning Oracle
	@$(call print_info,Running burning_oracle)
	./burning_oracle

test: build ##H Run all oracle smoke tests
	@$(call print_info,Running tests)
	./burning_oracle
	./turan_oracle
	./evasion_oracle
	./wmat_oracle
	@$(call print_success,All tests passed.)

build/burning: burning_oracle
build/turan: turan_oracle
build/evasion: evasion_oracle
build/wmat: wmat_oracle

burning_oracle: src/burning_oracle.cpp
	@$(call print_info,Building $@)
	$(CXX) $(CXXFLAGS) -o $@ $<
	@$(call print_success,Build complete.)

turan_oracle: src/turan_oracle.cpp
	@$(call print_info,Building $@)
	$(CXX) $(CXXFLAGS) -o $@ $<
	@$(call print_success,Build complete.)

evasion_oracle: src/evasion_oracle.cpp
	@$(call print_info,Building $@)
	$(CXX) $(CXXFLAGS) -o $@ $<
	@$(call print_success,Build complete.)

wmat_oracle: src/wmat_oracle.cpp
	@$(call print_info,Building $@)
	$(CXX) $(CXXFLAGS) -o $@ $<
	@$(call print_success,Build complete.)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Dev Tools
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LAKE_PKG_DIR ?= $(HOME)/.cache/lake/packages

define ensure_lake_packages
	@mkdir -p $(LAKE_PKG_DIR)
	@mkdir -p proofs/.lake
	@if [ ! -L proofs/.lake/packages ]; then \
		rm -rf proofs/.lake/packages; \
		ln -s $(LAKE_PKG_DIR) proofs/.lake/packages; \
	fi
endef

lean: ##H Build Lean 4 proofs
	@$(call print_info,Building Lean proofs)
	$(ensure_lake_packages)
	cd proofs && lake build
	@$(call print_success,Lean proofs verified.)

format: ##H Format all source files
	@$(call print_info,Formatting)
	find . -not -path '*/.lake/*' -name '*.md' -exec sed -i 's/[[:space:]]*$$//' {} +
	-prettier -w .
	-pre-commit run --all-files
	clang-format -i $(SRCS)
	@$(call print_success,Formatting complete.)

lint: ##H Lint C++ sources
	@$(call print_info,Linting)
	-cppcheck --std=c++17 --enable=warning,style,performance --quiet $(SRCS)
	-clang-tidy $(SRCS) --checks='*,-llvmlibc-*,-fuchsia-*,-altera-*,-boost-*,-llvm-*' -- $(CXXFLAGS)
	@$(call print_success,Lint complete.)

clean: ##H Remove build artifacts
	@$(call print_info,Cleaning)
	rm -f $(TARGETS) *.o
	@$(call print_success,Clean complete.)

help: _help
_help: ##H Show this help
	@printf "\nUsage: make <command>\n\n"
	@grep -E '^[a-zA-Z_/.-]+:.*?##H' $(MAKEFILE_LIST) | sort | sed 's/:.*##H /\t/' | expand -t 20 | sed 's/^/  /'
	@printf "\n"

vars: ##H Debug: Print project variables
	@$(foreach v,$(sort $(.VARIABLES)), \
		$(if $(filter file command line override,$(origin $(v))), \
			$(info $(v) = $($(v))) \
		)\
	)
