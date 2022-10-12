
build:
	@idf.py build

run:
	@make build
	@make flash
	@make monitor

flash:
	@idf.py flash
monitor:
	@idf.py monitor


.PHONY: build run flash monitor
