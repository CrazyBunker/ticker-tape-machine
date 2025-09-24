build:
	arduino-cli compile --fqbn esp32:esp32:esp32 --build-property build.flash_mode=dio --build-property build.flash_size=4MB -v --output-dir ./build .

deploy:
	arduino-cli upload -p 192.168.100.30 --upload-field password=admin --fqbn esp32:esp32:esp32  --build-path ./build --protocol network; \
	if [ $$? -ne 0 ]; then \
		arduino-cli upload -p /dev/ttyUSB0  --fqbn esp32:esp32:esp32  --build-path ./build; \
	fi
release:
	git tag -a v$(v) -m "Release $(v)"
	git push origin v$(v)