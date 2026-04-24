CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -Wall -Wextra -fopenmp -mavx512f -mavx512bw -mavx512dq

TARGETS = burning_oracle turan_oracle evasion_oracle wmat_oracle

all: $(TARGETS)

burning_oracle: src/burning_oracle.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

turan_oracle: src/turan_oracle.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

evasion_oracle: src/evasion_oracle.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

wmat_oracle: src/wmat_oracle.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

lean:
	cd proofs && lake build

format:
	clang-format -i src/*.cpp
	@echo "C++ formatting complete."

clean:
	rm -f $(TARGETS)
