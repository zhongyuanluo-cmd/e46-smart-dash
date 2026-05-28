## ADDED Requirements

### Requirement: GPIO output control
The system SHALL allow setting and clearing GPIO pins via the sysfs interface (`/sys/class/gpio/`), with at least one GPIO pin verified by controlling an external LED or logic analyzer.

#### Scenario: GPIO export and toggle
- **WHEN** the user exports a GPIO pin (e.g., `echo 26 > /sys/class/gpio/export`) and sets direction to "out"
- **THEN** writing "1" to the value file drives the pin high
- **AND** writing "0" to the value file drives the pin low
- **AND** the state change is confirmed by a logic analyzer or multimeter

### Requirement: GPIO input reading
The system SHALL read the logic level of GPIO pins configured as inputs via the sysfs interface.

#### Scenario: GPIO input reads pull-up
- **WHEN** a GPIO pin is exported and configured as input with internal pull-up enabled
- **THEN** reading the value file returns "1"
- **AND** connecting the pin to GND causes the value to read "0"

### Requirement: UART data transmission
The system SHALL transmit and receive data over UART interfaces at standard baud rates (9600, 115200, 1500000), verified by a loopback test (TX connected to RX).

#### Scenario: UART loopback at 115200 bps
- **WHEN** TX and RX pins of a UART interface are connected with a jumper wire
- **THEN** data written to `/dev/ttyS<n>` is read back identically
- **AND** no framing errors or overruns are reported

#### Scenario: UART at 1500000 bps (console)
- **WHEN** the UART console is configured at 1500000 baud
- **THEN** interactive shell input and output work correctly
- **AND** `stty -F /dev/ttyS<n>` reports `speed 1500000 baud`

### Requirement: SPI data transfer
The system SHALL perform full-duplex SPI data transfers via the spidev interface, verified by a loopback test (MOSI connected to MISO).

#### Scenario: SPI loopback at 10 MHz
- **WHEN** MOSI and MISO pins of an SPI bus are connected with a jumper wire
- **AND** a test program sends a known byte pattern via `ioctl(SPI_IOC_MESSAGE)`
- **THEN** the received data matches the transmitted data exactly
- **AND** no CRC or integrity errors occur over 1000 transfers

### Requirement: I2C device detection
The system SHALL scan the I2C bus and detect connected devices at their expected addresses using `i2cdetect`.

#### Scenario: I2C scan shows no errors
- **WHEN** the user runs `i2cdetect -y <bus_number>`
- **THEN** the scan completes without I2C controller errors
- **AND** the bus is confirmed functional (no all-FF ghost results due to missing pull-ups)
