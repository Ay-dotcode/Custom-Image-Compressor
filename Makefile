# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -O2 -w      # C++20, optimize, suppress warnings
LDFLAGS = -lpng                    # link with libpng

# Pattern rule: compile and run a .cpp file
%: %.cpp
	@echo "🔧 Compiling $<..."
	@$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS) && \
	echo "🔧 Compilation successful!" && \
	echo "🚀 Running $@..." && ./$(basename $@) || \
	(echo "❌ Compilation failed." && exit 1)

# Clean rule
clean:
	rm -f *.o *.out
