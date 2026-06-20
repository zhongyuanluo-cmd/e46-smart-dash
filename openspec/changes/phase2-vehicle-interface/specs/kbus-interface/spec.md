## ADDED Requirements

### Requirement: K-Bus UART configuration

The system SHALL communicate with the E46 K-Bus via a UART configured at 9600 baud, 8 data bits, even parity, 1 stop bit (8E1), using the TH3122.4 transceiver for 12V level conversion.

#### Scenario: UART opens successfully
- **WHEN** the K-Bus UART device (ttySx) is opened by the daemon
- **THEN** the file descriptor SHALL be valid and ready for read/write

#### Scenario: Baud rate verification
- **WHEN** the UART is configured
- **THEN** `stty -F /dev/ttySx` SHALL show speed 9600 baud, cs8, parenb, -parodd

### Requirement: K-Bus byte-level frame reception

The system SHALL receive individual K-Bus byte frames from the UART, where each frame starts with a source address byte, followed by a length byte, destination address byte, data bytes, and a checksum.

#### Scenario: Valid frame received
- **WHEN** a complete K-Bus frame arrives on the UART with correct checksum
- **THEN** the system SHALL parse source address, length, destination address, and data payload
- **THEN** the parsed frame SHALL be passed to the message dispatcher

#### Scenario: Checksum failure
- **WHEN** a K-Bus frame arrives with incorrect checksum
- **THEN** the system SHALL discard the frame
- **THEN** the system SHALL log a warning with the source address (if parsable)

#### Scenario: Partial frame timeout
- **WHEN** the start of a K-Bus frame is received but remaining bytes do not arrive within 50ms
- **THEN** the system SHALL discard the partial frame and reset to idle state

### Requirement: K-Bus idle detection

The system SHALL detect K-Bus bus idle state (no activity for >2 byte times) before initiating transmission to avoid bus collisions.

#### Scenario: Transmission with collision avoidance
- **WHEN** the system needs to transmit a K-Bus message
- **THEN** it SHALL wait for bus idle (no bytes received for >2.1ms at 9600 baud)
- **THEN** it SHALL then transmit the complete frame

### Requirement: TH3122.4 electrical isolation

The TH3122.4 transceiver SHALL provide electrical isolation between the 12V K-Bus and the 3.3V RK3566 UART, preventing damage from bus voltage transients.

#### Scenario: Bus fault protection
- **WHEN** the K-Bus line experiences a voltage spike up to 40V
- **THEN** the RK3566 UART pins SHALL not be exposed to voltages exceeding 3.6V
