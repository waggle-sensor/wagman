FROM buildpack-deps
COPY . .
RUN tar -C /usr/local/bin -xvf arduino-cli_latest_Linux_64bit.tar.gz
RUN arduino-cli core update-index && \
    arduino-cli core install arduino:avr arduino:sam && \
    arduino-cli lib install SD OneWire DueTimer
RUN ./install-custom-boards
RUN git clone https://github.com/waggle-sensor/eWaggle /root/Arduino/libraries/waggle
