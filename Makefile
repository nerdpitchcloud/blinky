.PHONY: all clean agent collector install help

all:
	@mkdir -p build
	@cd build && cmake .. && $(MAKE) -j$$(nproc)

agent:
	@mkdir -p build
	@cd build && cmake -DBUILD_COLLECTOR=OFF .. && $(MAKE) -j$$(nproc)

collector:
	@mkdir -p build
	@cd build && cmake -DBUILD_AGENT=OFF .. && $(MAKE) -j$$(nproc)

clean:
	@rm -rf build

install:
	@./install.sh

help:
	@echo "Blinky Build System"
	@echo "==================="
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build both agent and collector (default)"
	@echo "  agent      - Build only the agent"
	@echo "  collector  - Build only the collector"
	@echo "  clean      - Remove build directory"
	@echo "  install    - Install binaries (requires root)"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "After building, binaries are located in:"
	@echo "  build/agent/blinky-agent"
	@echo "  build/collector/blinky-collector"
