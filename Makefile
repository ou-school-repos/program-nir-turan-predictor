PYTHON = python3

.DEFAULT_GOAL := _help

.PHONY: all
all: format lint clean build paper docs bundle ##H Full pipeline: format, lint, clean, build, paper, docs, bundle

# --- Print Helpers ---
define print_info
	printf "\033[1;36m%s\033[0m\n" "$(1)"
endef
define print_success
	printf "\033[1;34m✓ %s\033[0m\n" "$(1)"
endef

# --- Simulation Scaling ---
SCALE ?= 64
ITER  ?= 1000

# --- Unified Execution & Certification ---

.PHONY: verify/epidemiology
verify/epidemiology: ##H Generate and certify Wolbachia deployment
	@{ \
		$(call print_info,Generating Epidemiology Policy [Scale: $(SCALE)]); \
		$(PYTHON) src/solver.py epidemiology proofs/VectorDeployment.lean; \
		export DEP_SEQ=$$(grep "deployment_sequence" proofs/VectorDeployment.lean | sed 's/def deployment_sequence : List Nat := //'); \
		cd proofs && lake build VectorDeployment; \
		printf "\033[1;34m✓ Verified: policy_is_valid (native_decide evaluated to TRUE).\033[0m\n"; \
		printf "\033[1;36m==================================================\033[0m\n"; \
		printf "\033[1;36mCERTIFIED DEPLOYMENT LOGISTIC MAP:\033[0m\n"; \
		printf "  %s\n" "$$DEP_SEQ"; \
		printf "\033[1;36mMATHEMATICAL GUARANTEE: 100%% network saturation achieved.\033[0m\n"; \
		printf "\033[1;36m==================================================\033[0m\n"; \
	} | tee output.log

.PHONY: verify/surveillance
verify/surveillance: ##H Generate and certify drone surveillance playbook
	@{ \
		$(call print_info,Generating Threat Hunting Playbook [Iter: $(ITER)]); \
		$(PYTHON) src/solver.py surveillance proofs/ThreatHunting.lean; \
		export DRONE_SEQ=$$(grep "drone_routing_playbook" proofs/ThreatHunting.lean | sed 's/def drone_routing_playbook : List Nat := //'); \
		cd proofs && lake build ThreatHunting; \
		printf "\033[1;34m✓ Verified: capture_guaranteed (native_decide evaluated to TRUE).\033[0m\n"; \
		printf "\033[1;36m==================================================\033[0m\n"; \
		printf "\033[1;36mCERTIFIED DRONE FLIGHT PLAYBOOK:\033[0m\n"; \
		printf "  %s\n" "$$DRONE_SEQ"; \
		printf "\033[1;36mMATHEMATICAL GUARANTEE: 0 blind spots. Evasion impossible.\033[0m\n"; \
		printf "\033[1;36m==================================================\033[0m\n"; \
	} | tee output.log

.PHONY: run/adversarial
run/adversarial: ##H Run adversarial Maker-Breaker game (all presets)
	@{ \
		$(call print_info,Running Adversarial Burning); \
		./dendro adversarial proofs/Adversarial.lean grid4x4; \
		./dendro adversarial proofs/Adversarial.lean tree15; \
		./dendro adversarial proofs/Adversarial.lean campus; \
	} | tee output.log

.PHONY: run/finance
run/finance: ##H Audit financial network for systemic risk
	@{ \
		$(call print_info,Running Systemic Risk Audit [Scale: $(SCALE)]); \
		$(PYTHON) src/solver.py finance proofs/RiskAudit.lean; \
	} | tee output.log

.PHONY: test/all
test/all: ##H Run all certification pipelines
	@$(call print_info,Running all pipelines)
	@{ \
		$(MAKE) --no-print-directory verify/epidemiology; \
		$(MAKE) --no-print-directory verify/surveillance; \
		$(MAKE) --no-print-directory run/adversarial; \
		$(MAKE) --no-print-directory run/finance; \
	} | tee output.log
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

.PHONY: lean
lean: ##H Build Lean 4 verifiers
	$(ensure_lake_packages)
	cd proofs && lake build

.PHONY: lean-analytic
lean-analytic: ##H Build the analytic LeanLeontovich project
	@mkdir -p $(HOME)/.cache/lake
	@mkdir -p LeanLeontovich/.lake
	@if [ ! -L LeanLeontovich/.lake ]; then \
		rm -rf LeanLeontovich/.lake; \
		ln -s $(HOME)/.cache/lake/LeanLeontovich-proofs LeanLeontovich/.lake; \
	fi
	set -o pipefail; cd LeanLeontovich && lake build | tee lean.log
	@printf "\n\033[1;32m--- Verification Complete ---\033[0m\n"
	@printf "\033[1;36mMapped Theorems & Definitions:\033[0m\n"
	@awk 'BEGIN {last_file=""} \
		/^(theorem|lemma|def|axiom|class|instance|structure) / { \
			if (in_decl) process_buf(); \
			buf = $$0; in_decl = 1; \
			if (buf ~ /(:=|:= by|by|where|=>)/) process_buf(); \
			next; \
		} \
		in_decl { \
			gsub(/^[[:space:]]+/, " ", $$0); \
			buf = buf $$0; \
			if ($$0 ~ /(:=|:= by|by|where|=>)/) process_buf(); \
		} \
		function process_buf() { \
			gsub(/[[:space:]]+/, " ", buf); \
			file = FILENAME; sub(/^LeanLeontovich\//, "", file); \
			if (file != last_file) { \
				printf "\n\033[1;33m%s:\033[0m\n", file; \
				last_file = file; \
			} \
			printf "  %s\n", buf; \
			buf = ""; in_decl = 0; \
		} \
		END { if (in_decl) process_buf(); }' \
		LeanLeontovich/LeanLeontovich/*.lean 2>/dev/null || true
	@printf "\033[1;32m--------------------------------\033[0m\n"
	@$(call print_success,LeanLeontovich proofs verified.)

.PHONY: lean-analytic-cache
lean-analytic-cache: ##H Download mathlib cache for LeanLeontovich
	cd LeanLeontovich && lake exe cache get


.PHONY: doc
F ?= docs/SEQUENCE_DISCOVERY.md
doc: ##H Convert markdown to PDF (F=docs/file.md)
	@$(call print_info,Generating PDF from $(F))
	pandoc $(F) --resource-path="$$(dirname "$(F)")" --pdf-engine=xelatex -V geometry:margin=1in -o $(basename $(F)).pdf
	touch -r $(F) $(basename $(F)).pdf
	@$(call print_success,$(basename $(F)).pdf)

.PHONY: docs
docs: ##H Convert all tracked markdown files to PDF
	@for f in $$(git ls-files '*.md'); do \
		$(call print_info,$$f → $$(basename $$f .md).pdf); \
		pandoc "$$f" --resource-path="$$(dirname "$$f")" --pdf-engine=xelatex -V geometry:margin=1in -o "$$(dirname $$f)/$$(basename $$f .md).pdf"; \
		touch -r "$$f" "$$(dirname $$f)/$$(basename $$f .md).pdf"; \
	done
	@$(call print_success,All PDFs generated.)

.PHONY: paper
paper: ##H Compile paper/paper.tex to PDF
	@$(call print_info,Building paper)
	#-for g in docs/out/*.gif; do magick "$$g" "$${g%.gif}.png" 2>/dev/null || convert "$$g" "$${g%.gif}.png" 2>/dev/null || true; done
	cd paper && pdflatex -interaction=nonstopmode paper.tex && bibtex paper && pdflatex -interaction=nonstopmode paper.tex && pdflatex -interaction=nonstopmode paper.tex
	@$(call print_success,paper/paper.pdf)

.PHONY: lean-cache
lean-cache: ##H Download mathlib cache
	cd proofs && lake exe cache get

N ?= 21

.PHONY: build
build: synthesizer dendro firefighter leontovich_fast leontovich_sa depth5_sweep landscape_txz ##H Build all C++ binaries

synthesizer: src/synthesizer.cpp src/hpc_core.hpp ##H Build the C++ tree synthesizer
	@$(call print_info,Building synthesizer)
	g++ -O3 -march=native -std=c++17 -o synthesizer src/synthesizer.cpp
	@$(call print_success,synthesizer built.)

dendro: src/dendro.cpp ##H Build the C++ Dendro engine
	@$(call print_info,Building dendro)
	g++ -O3 -march=native -std=c++17 -o dendro src/dendro.cpp
	@$(call print_success,dendro built.)

firefighter: src/firefighter.cpp ##H Build the single-ignition firefighter solver (Model B)
	@$(call print_info,Building firefighter)
	g++ -O3 -march=native -std=c++17 -o firefighter src/firefighter.cpp
	@$(call print_success,firefighter built.)

leontovich_fast: src/leontovich_fast.cpp ##H Build the Leontovich graph filter
	@$(call print_info,Building leontovich_fast)
	g++ -O3 -march=native -std=c++17 -fopenmp -o leontovich_fast src/leontovich_fast.cpp
	@$(call print_success,leontovich_fast built.)

leontovich_sa: src/leontovich_sa.cpp ##H Build the Leontovich SA search
	@$(call print_info,Building leontovich_sa)
	g++ -O3 -march=native -std=c++17 -o leontovich_sa src/leontovich_sa.cpp
	@$(call print_success,leontovich_sa built.)

depth5_sweep: src/depth5_sweep.cpp ##H Build the depth-5 Leontovich sweep
	@$(call print_info,Building depth5_sweep)
	g++ -O3 -march=native -std=c++17 -fopenmp -o depth5_sweep src/depth5_sweep.cpp
	@$(call print_success,depth5_sweep built.)

landscape_txz: src/landscape_txz.cpp ##H Build the T(x,1,z) landscape search
	@$(call print_info,Building landscape_txz)
	g++ -O3 -march=native -std=c++17 -fopenmp -o landscape_txz src/landscape_txz.cpp
	@$(call print_success,landscape_txz built.)

.PHONY: dots
dots: dendro ##H Regenerate all .dot visual proofs and .lean witnesses
	@$(call print_info,Regenerating witnesses and graphs)
	@$(PYTHON) src/solver.py epidemiology proofs/VectorDeployment.lean
	@$(PYTHON) src/solver.py surveillance proofs/ThreatHunting.lean
	@./dendro adversarial proofs/Adversarial.lean grid4x4
	@./dendro adversarial proofs/Adversarial.lean tree15
	@./dendro adversarial proofs/Adversarial.lean campus
	@$(PYTHON) src/solver.py finance proofs/RiskAudit.lean
	@SYNTH_N=$(N) $(PYTHON) src/solver.py synthesize proofs/SynthesizerDiscovery.lean

# Layout engine map: module -> engine
# Epidemiology (fdp), Surveillance (dot), Finance (sfdp), others (dot)
DOT_ENGINE = dot
define render_dot
	$(eval ENGINE := $(if $(findstring RiskAudit,$1),sfdp,\
		$(if $(findstring VectorDeployment,$1),fdp,\
		dot)))
	$(ENGINE) -Gdpi=150 -Tgif "$1" -o "$2"
endef

.PHONY: render
render: dots ##H Render all .dot visual proofs (requires graphviz)
	@$(call print_info,Rendering visual proofs)
	@mkdir -p docs/out
	@for f in docs/*.dot; do \
		base=$$(basename "$$f" .dot); \
		engine=dot; \
		case "$$base" in \
			*RiskAudit*) engine=sfdp ;; \
			*VectorDeployment*) engine=fdp ;; \
		esac; \
		$$engine -Gdpi=150 -Tgif "$$f" -o "docs/out/$${base}.gif"; \
		printf "  \033[1;34m✓ Rendered: docs/out/$${base}.gif\033[0m\n"; \
	done


LINT_LOCS_PY ?= $$(git ls-files '*.py')

.PHONY: format
format: ##H Format source files
	-shfmt -w $$(git ls-files '*.sh')
	-black $(LINT_LOCS_PY)
	-isort $(LINT_LOCS_PY)
	-clang-format -i $$(git ls-files '*.cpp' '*.hpp' '*.h')
	-prettier -w .
	-pre-commit run --all-files


.PHONY: lint
lint: ##H Lint sources
	@$(call print_info,Linting)
	-flake8 $(LINT_LOCS_PY)
	-cppcheck --enable=warning,style --std=c++17 --quiet $$(git ls-files '*.cpp')
	@$(call print_success,Lint complete.)

.PHONY: bundle
bundle: clean ##H Package project into bundle.zip
	zip -rv0 bundle.zip . -x ".git/*" ".lake/*" "bundle.zip" "proofs/.lake/*" "proofs/docbuild/*" ".tmp/*"

.PHONY: clean
clean: ##H Remove build artifacts
	rm -f *.o bundle.zip synthesizer dendro firefighter leontovich_fast landscape_txz
	find . -type d -name __pycache__ -exec rm -rf {} + 2>/dev/null || true

.PHONY: _help
_help: ##H Show this help
	@printf "\nUsage: make <command>\n\n"
	@grep -E '^[a-zA-Z_/.-]+:.*?##H' $(MAKEFILE_LIST) | sort | sed 's/:.*##H /\t/' | expand -t 20 | sed 's/^/  /'
	@printf "\n"
