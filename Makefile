CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -Wall -Wextra -Wpedantic -fopenmp -mavx512f -mavx512bw -mavx512dq
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

.PHONY: all build run test clean format lint lean lean-cache bundle _help solve_vector_ecology allocate_spectrum audit_finance hunt_threat

all: build

build: $(TARGET) ##H Build the unified hybrid oracle

$(TARGET): $(SRC)
	@$(call print_info,Building unified oracle)
	$(CXX) $(CXXFLAGS) -o $@ $<
	@$(call print_success,Build complete.)

run: build ##H Run the oracle in default mode
	@$(call print_info,Running oracle)
	./$(TARGET)

test: build ##H Run unified smoke tests
	@$(call print_info,Running tests)
	./$(TARGET) test_all
	@$(call print_success,Tests passed.)

# --- Certification Pipelines ---
solve_vector_ecology: build ##H Generate and certify Wolbachia deployment
	@$(call print_info,Generating Epidemiology Policy)
	./$(TARGET) epidemiology proofs/VectorDeployment.lean
	cd proofs && lake build VectorDeployment

allocate_spectrum: build ##H Stress-test 6G frequency allocation
	@$(call print_info,Generating 6G Signal Audit)
	./$(TARGET) spectrum proofs/SignalAudit.lean
	cd proofs && lake build SignalAudit

audit_finance: build ##H Audit financial network for systemic risk
	@$(call print_info,Generating Systemic Risk Audit)
	./$(TARGET) finance proofs/RiskAudit.lean
	cd proofs && lake build RiskAudit

hunt_threat: build ##H Generate and certify drone surveillance playbook
	@$(call print_info,Generating Threat Hunting Playbook)
	./$(TARGET) surveillance proofs/ThreatHunting.lean
	cd proofs && lake build ThreatHunting

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
