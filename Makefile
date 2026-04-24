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

.PHONY: all build run test clean format lint lean bundle _help

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

hunt_threat: build ##H Generate and certify drone surveillance playbook
	@$(call print_info,Generating Threat Hunting Playbook)
	./$(TARGET) surveillance proofs/ThreatHunting.lean
	cd proofs && lake build ThreatHunting

allocate_spectrum: build ##H Stress-test 6G frequency allocation
	@$(call print_info,Generating 6G Signal Audit)
	./$(TARGET) spectrum proofs/SignalAudit.lean
	cd proofs && lake build SignalAudit

audit_finance: build ##H Audit financial network for systemic risk
	@$(call print_info,Generating Systemic Risk Audit)
	./$(TARGET) finance proofs/RiskAudit.lean
	cd proofs && lake build RiskAudit

format: ##H Format source files
	clang-format -i $(SRC)
	-prettier -w .
	-pre-commit run --all-files

lint: ##H Lint C++ sources
	-cppcheck --enable=all --quiet $(SRC)
	-clang-tidy $(SRC) -- $(CXXFLAGS)

lean: ##H Build Lean 4 verifiers
	cd proofs && lake build

bundle: clean ##H Package project into bundle.zip
	zip -r bundle.zip . -x ".git/*" ".lake/*" "bundle.zip" "proofs/.lake/*"

clean: ##H Remove build artifacts
	rm -f $(TARGET) *.o bundle.zip

_help: ##H Show this help
	@printf "\nUsage: make <command>\n\n"
	@grep -E '^[a-zA-Z_/.-]+:.*?##H' $(MAKEFILE_LIST) | sort | sed 's/:.*##H /\t/' | expand -t 20 | sed 's/^/  /'
	@printf "\n"
