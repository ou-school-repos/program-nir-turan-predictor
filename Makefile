CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -Wall -Wextra -Wpedantic -fopenmp -mavx2
LDFLAGS =

TARGET = oracle
SRC = src/oracle.cpp

.DEFAULT_GOAL := _help

# --- Print Helpers ---
define print_info
	printf "\033[1;36m%s\033[0m\n" "$(1)"
endef
define print_success
	printf "\033[1;34m✓ %s\033[0m\n" "$(1)"
endef

.PHONY: all build clean format lint lean lean-cache bundle _help \
        verify/epidemiology verify/surveillance run/spectrum run/finance \
        test/all

all: build

build: $(TARGET) ##H Build the unified hybrid oracle

$(TARGET): $(SRC)
	@$(call print_info,Building unified oracle)
	$(CXX) $(CXXFLAGS) -o $@ $<
	@$(call print_success,Build complete.)

# --- Simulation Scaling ---
SCALE ?= 64
ITER  ?= 1000

# --- Unified Execution & Certification ---

verify/epidemiology: build ##H Generate and certify Wolbachia deployment (Lean focused)
	@$(call print_info,Generating Epidemiology Policy [Scale: $(SCALE)])
	./$(TARGET) epidemiology proofs/VectorDeployment.lean $(SCALE) $(ITER)
	cd proofs && lake build VectorDeployment

verify/surveillance: build ##H Generate and certify drone surveillance playbook (Lean focused)
	@$(call print_info,Generating Threat Hunting Playbook [Iter: $(ITER)])
	./$(TARGET) surveillance proofs/ThreatHunting.lean $(SCALE) $(ITER)
	cd proofs && lake build ThreatHunting

run/spectrum: build ##H Stress-test 6G frequency allocation (C++ benchmark)
	@$(call print_info,Running 6G Signal Audit [Iter: $(ITER)])
	./$(TARGET) spectrum proofs/SignalAudit.lean $(SCALE) $(ITER)

run/finance: build ##H Audit financial network for systemic risk (C++ benchmark)
	@$(call print_info,Running Systemic Risk Audit [Scale: $(SCALE)])
	./$(TARGET) finance proofs/RiskAudit.lean $(SCALE) $(ITER)

test/all: build ##H Run all certification pipelines
	@$(call print_info,Running all pipelines)
	$(MAKE) verify/epidemiology
	$(MAKE) verify/surveillance
	$(MAKE) run/spectrum
	$(MAKE) run/finance
	@$(call print_success,All pipelines verified.)

# --- Dev Tools ---
LAKE_PKG_DIR ?= $(HOME)/.cache/lake/packages

define ensure_lake_packages
	@mkdir -p $(LAKE_PKG_DIR)
	@mkdir -p proofs/.lake
	@if [ ! -L proofs/.lake/packages ]; then \
		rm -rf proofs/.lake/packages; \
		ln -s $(LAKE_PKG_DIR) proofs/.lake/packages; \
	fi
endef

lean: ##H Build Lean 4 verifiers
	$(ensure_lake_packages)
	cd proofs && lake build

lean-cache: ##H Download mathlib cache
	cd proofs && lake exe cache get

format: ##H Format source files
	clang-format -i $(SRC)
	-prettier -w .
	-pre-commit run --all-files

lint: ##H Lint C++ sources
	@$(call print_info,Linting)
	-cppcheck --std=c++17 --enable=warning,style,performance --quiet $(SRC)
	-clang-tidy $(SRC) --checks='*,-llvmlibc-*,-fuchsia-*,-altera-*,-boost-*,-llvm-*' -- $(CXXFLAGS)
	@$(call print_success,Lint complete.)

bundle: clean ##H Package project into bundle.zip
	zip -r bundle.zip . -x ".git/*" ".lake/*" "bundle.zip" "proofs/.lake/*"

clean: ##H Remove build artifacts
	rm -f $(TARGET) *.o bundle.zip

_help: ##H Show this help
	@printf "\nUsage: make <command>\n\n"
	@grep -E '^[a-zA-Z_/.-]+:.*?##H' $(MAKEFILE_LIST) | sort | sed 's/:.*##H /\t/' | expand -t 20 | sed 's/^/  /'
	@printf "\n"
