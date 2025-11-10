.PHONY: help build upload monitor upload-monitor clean update-deps test-symbols web-build web-update docs docs-clean docs-open all

help:
	@echo "CYD Crypto Ticker - Makefile Commands"
	@echo ""
	@echo "Building & Uploading:"
	@echo "  make build           - Build the firmware"
	@echo "  make upload          - Upload firmware to device"
	@echo "  make monitor         - Monitor serial output"
	@echo "  make upload-monitor  - Upload and start monitoring (most common)"
	@echo "  make clean           - Clean build files"
	@echo "  make all             - Clean, build, upload, and monitor"
	@echo ""
	@echo "Development:"
	@echo "  make update-deps     - Update library dependencies"
	@echo "  make test-symbols    - Test Binance symbol discovery"
	@echo ""
	@echo "Web Interface:"
	@echo "  make web-build       - Build PicoCSS styles"
	@echo "  make web-update      - Update embedded CSS in NetworkManager.cpp"
	@echo ""
	@echo "Documentation:"
	@echo "  make docs            - Generate Doxygen documentation"
	@echo "  make docs-clean      - Clean generated documentation"
	@echo "  make docs-open       - Generate and open documentation in browser"
	@echo ""

build:
	uv tool run platformio run

upload:
	uv tool run platformio run -t upload

monitor:
	uv tool run platformio run -t monitor

upload-monitor:
	uv tool run platformio run -t upload -t monitor

clean:
	uv tool run platformio run -t clean

update-deps:
	uv tool run platformio lib update

test-symbols:
	uv run --with requests python3 find_binance_symbols.py BTC ETH SOL

web-build:
	cd picocss && npm run build

web-update:
	cd picocss && npm run update-cpp

docs:
	./generate_docs.sh

docs-clean:
	./generate_docs.sh clean

docs-open:
	./generate_docs.sh open

all: clean build upload-monitor
