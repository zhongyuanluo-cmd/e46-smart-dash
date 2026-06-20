## ADDED Requirements

### Requirement: Multi-source data collection

The system SHALL collect data from CAN bus, K-Bus, and ADC simultaneously using epoll-based I/O multiplexing, without blocking any one source.

#### Scenario: All sources active
- **WHEN** CAN frames arrive, K-Bus bytes arrive, and ADC timer fires concurrently
- **THEN** all events SHALL be processed without any source being starved
- **THEN** no data SHALL be lost due to blocking on another source

#### Scenario: One source disconnected
- **WHEN** the CAN interface is down (can0 link down) but K-Bus and ADC are active
- **THEN** the daemon SHALL continue collecting from K-Bus and ADC without errors
- **THEN** VehicleData.can_status SHALL indicate "disconnected"

### Requirement: DBus data publication

The system SHALL publish decoded vehicle data on the session DBus at interface `com.e46.can1.VehicleData`, with properties for each data field.

#### Scenario: DBus property query
- **WHEN** a DBus client calls `org.freedesktop.DBus.Properties.Get` for "RPM" on `com.e46.can1.VehicleData`
- **THEN** the daemon SHALL return the current RPM value as a 32-bit unsigned integer

#### Scenario: DBus properties update signal
- **WHEN** a vehicle data field changes (e.g., RPM from 800 to 2500)
- **THEN** the daemon SHALL emit `org.freedesktop.DBus.Properties.PropertiesChanged` signal

### Requirement: Systemd service integration

The system SHALL provide a systemd service unit (`can-gateway.service`) for automatic startup and lifecycle management.

#### Scenario: Service starts at boot
- **WHEN** the system boots and reaches multi-user.target
- **THEN** `can-gateway.service` SHALL be started automatically
- **THEN** `systemctl status can-gateway` SHALL show active (running)

#### Scenario: Service restart on failure
- **WHEN** the daemon process crashes unexpectedly
- **THEN** systemd SHALL restart the service after 5 seconds
- **THEN** restart attempts SHALL be logged to the system journal

### Requirement: Configurable via JSON

The system SHALL read its configuration from a JSON file at `/etc/e46/can-gateway.json`, including CAN interface name, bitrate, K-Bus UART device, ADC I2C bus/address, and sampling intervals.

#### Scenario: Configuration file present
- **WHEN** `/etc/e46/can-gateway.json` contains valid JSON with CAN and K-Bus settings
- **THEN** the daemon SHALL use those settings on startup

#### Scenario: Configuration file missing
- **WHEN** `/etc/e46/can-gateway.json` does not exist
- **THEN** the daemon SHALL use hardcoded defaults and log a warning

### Requirement: Graceful shutdown

The system SHALL close all file descriptors, stop timers, and release DBus names cleanly on SIGTERM or SIGINT.

#### Scenario: SIGTERM handling
- **WHEN** `systemctl stop can-gateway` is executed
- **THEN** the daemon SHALL close CAN socket, UART, and ADC file descriptors
- **THEN** the daemon SHALL exit with code 0 within 3 seconds

### Requirement: vcan mode for testing

The system SHALL support a `--vcan` flag that uses vcan (virtual CAN) instead of a physical CAN interface for integration testing without hardware.

#### Scenario: vcan test mode
- **WHEN** the daemon is started with `--vcan`
- **THEN** it SHALL use `vcan0` as the CAN interface
- **THEN** frames injected via `cansend vcan0` SHALL be decoded and published on DBus
