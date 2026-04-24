SHELL:=/bin/bash
.DEFAULT_GOAL := help
.SHELLFLAGS = -ec

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Variables
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -mavx512f -mavx512bw -mavx512dq -fopenmp
LDFLAGS  ?=

SRC_BURNING = src/burning_oracle.cpp
BIN_BURNING = burning_oracle

SRC_TURAN   = src/turan_oracle.cpp
BIN_TURAN   = turan_oracle

SRC_EVASION = src/evasion_oracle.cpp
BIN_EVASION = evasion_oracle

SRC_WMAT    = src/wmat_oracle.cpp
BIN_WMAT    = wmat_oracle

# Original legacy sources for reference/repurposing
SRC_OPT     = src/arrangementoptimized.cpp
BIN_OPT     = arrangementoptimized
SRC_PRED    = src/predict.cpp
BIN_PRED    = predict

SRCS = $(SRC_BURNING) $(SRC_TURAN) $(SRC_EVASION) $(SRC_WMAT) $(SRC_OPT) $(SRC_PRED)

# Build modes
OPTFLAGS  ?= -O3 -march=native
DBGFLAGS  ?= -g -O0 -fsanitize=address,undefined

# nauty (canonical graph labeling)
NAUTY_CFLAGS = -I/usr/include/nauty
NAUTY_LIBS   = -lnauty

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Help
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.PHONY: help
help: ##H Show this help
	@printf "\nUsage: make <command>\n\n"
	@grep -E '^[a-zA-Z_/.-]+:.*?##H' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?##H "}; {printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2}'
	@printf "\n"

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
.PHONY: build
build: build/burning build/turan build/evasion build/wmat ##H @Build Compile all Phase Oracles

.PHONY: build/burning
build/burning: ##H @Build Compile Graph Burning Oracle (Phase 1)
	@$(call print_info,Building $(BIN_BURNING))
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $(BIN_BURNING) $(SRC_BURNING)
	@$(call print_success,Build complete.)

.PHONY: build/turan
build/turan: ##H @Build Compile Turan Oracle (Phase 2)
	@$(call print_info,Building $(BIN_TURAN))
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $(BIN_TURAN) $(SRC_TURAN)
	@$(call print_success,Build complete.)

.PHONY: build/evasion
build/evasion: ##H @Build Compile Pursuit-Evasion Oracle (Phase 3)
	@$(call print_info,Building $(BIN_EVASION))
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $(BIN_EVASION) $(SRC_EVASION)
	@$(call print_success,Build complete.)

.PHONY: build/wmat
build/wmat: ##H @Build Compile WMat Oracle (Phase 4)
	@$(call print_info,Building $(BIN_WMAT))
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $(BIN_WMAT) $(SRC_WMAT)
	@$(call print_success,Build complete.)

.PHONY: build/legacy
build/legacy: ##H @Build Compile legacy Arrangement Graph tools
	@$(call print_info,Building legacy tools)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $(NAUTY_CFLAGS) $(LDFLAGS) -o $(BIN_OPT) $(SRC_OPT) $(NAUTY_LIBS)
	$(CXX) $(CXXFLAGS) -O3 $(LDFLAGS) -o $(BIN_PRED) $(SRC_PRED)
	@$(call print_success,Legacy build complete.)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Lean 4 Proofs
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

.PHONY: lean
lean: ##H @Build Build Lean 4 proofs (proofs/)
	@$(call print_info,Building Lean proofs)
	$(ensure_lake_packages)
	cd proofs && lake build | tee lean.log
	@$(call print_success,Lean proofs verified.)

.PHONY: clean
clean: ##H @General Remove build artifacts
	@$(call print_info,Cleaning)
	rm -f $(BIN_BURNING) $(BIN_TURAN) $(BIN_EVASION) $(BIN_WMAT) $(BIN_OPT) $(BIN_PRED) *.o
	@$(call print_success,Clean complete.)

