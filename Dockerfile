FROM arduinoci/ci-arduino-cli:v1.0.3-0.0.15
COPY . /sketch/ticker_machine
RUN arduino-cli core install esp32:esp32 &&\
    arduino-cli compile \
    -b esp32:esp32:esp32 \
    --build-property "build.flash_mode=dio,build.flash_size=4MB" \
    --output-dir /sketch/build /sketch/ticker_machine
    

