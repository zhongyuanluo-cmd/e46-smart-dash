## ADDED Requirements

### Requirement: MCP2515 DTBO single-CAN with vdd-supply

The system SHALL provide a Device Tree overlay that enables a single MCP2515 CAN controller on SPI1, including a vdd-supply reference and high_speed pinctrl state.

#### Scenario: DTBO compiles without errors
- **WHEN** `dtc -@ -I dts -O dtb mcp2515_can0.dts` is executed
- **THEN** compilation SHALL succeed with no errors

#### Scenario: DTBO enables single CAN interface
- **WHEN** the overlay is loaded via configfs or armbianEnv.txt
- **THEN** exactly one MCP2515 device SHALL be registered (can@0), and no can@1 SHALL exist

#### Scenario: Driver probe succeeds
- **WHEN** MCP2515 hardware is physically connected and overlay is loaded
- **THEN** `dmesg` SHALL show "mcp251x spi1.0: MCP2515 successfully initialized"
- **THEN** `ip link show can0` SHALL display the can0 interface

#### Scenario: High-speed pinctrl is applied
- **WHEN** SPI1 initiates a data transfer
- **THEN** the pinmux SHALL switch to `spi1m1_pins_hs` (M1 high-speed state) for the duration of the transfer

### Requirement: SocketCAN network interface

The system SHALL expose the MCP2515 as a standard SocketCAN network interface (`can0`) supporting 500kbps bitrate.

#### Scenario: CAN interface comes up
- **WHEN** `sudo ip link set can0 type can bitrate 500000` followed by `sudo ip link set up can0` is executed
- **THEN** `ip link show can0` SHALL display state UP
- **THEN** the bitrate SHALL be 500000

#### Scenario: CAN frame send via SocketCAN
- **WHEN** `cansend can0 123#DEADBEEF` is executed with a properly connected CAN bus
- **THEN** the frame SHALL be transmitted on the PT-CAN bus

#### Scenario: CAN frame receive via SocketCAN
- **WHEN** a CAN frame arrives on the PT-CAN bus
- **THEN** `candump can0` SHALL display the frame with correct ID, DLC, and data

### Requirement: MCP2515 module termination resistor removal

The MCP2515+TJA1050 module SHALL have its onboard 120Ω termination resistor physically removed before connecting to the E46 PT-CAN bus.

#### Scenario: Termination resistor removed
- **WHEN** the module is inspected before installation
- **THEN** the 120Ω resistor between CAN_H and CAN_L on the module SHALL be physically absent

### Requirement: SPI1 pin mapping for MCP2515

The system SHALL use the following SPI1 M1 pin assignments for MCP2515:
- MOSI: Pin 19 (GPIO3_C1)
- MISO: Pin 21 (GPIO3_C2)
- SCLK: Pin 23 (GPIO3_C3)
- CS: Pin 24 (GPIO3_A4, software-controlled)
- INT: Pin 16 (GPIO3_A1, falling edge interrupt)

#### Scenario: Pin mapping matches physical wiring
- **WHEN** MCP2515 module is wired according to the pin mapping
- **THEN** all SPI signals SHALL reach the correct MCP2515 pins
