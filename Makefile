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

clean:
	rm -rf build

list:
	echo "=== Структура директорий ==="
	tree -L 3 -a -I .git

	# Перечисляем все файлы и выводим содержимое текстовых
	echo "=== Содержимое файлов ==="
	find . -type f ! -path '*/.git/*' ! -name '*.bin' ! -name '*.elf' ! -name '*.o' -print0 | xargs -0 -I {} bash -c 'echo "Файл: {}"; if file "{}" | grep -q text; then cat "{}"; else echo "(Binary файл, пропущен)"; fi; echo -e "\n---\n"'

	# Очистка
	cd ..
	rm -rf $TEMP_DIR
	echo "Скрипт завершён. Скопируйте вывод выше и отправьте."

all: clean build deploy