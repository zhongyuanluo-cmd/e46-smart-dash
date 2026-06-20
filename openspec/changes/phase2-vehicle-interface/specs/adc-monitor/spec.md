## ADDED Requirements

### Requirement: I2C ADS1115 ADC integration

The system SHALL read battery voltage via an external ADS1115 16-bit ADC connected to the I2C bus, using a voltage divider to scale the automotive 12V (nominal, 14.4V charging) to the ADC's input range.

#### Scenario: ADS1115 detected on I2C bus
- **WHEN** `i2cdetect -y <bus>` is executed
- **THEN** the ADS1115 SHALL respond at its configured address (one of 0x48-0x4B)

#### Scenario: Voltage reading accuracy
- **WHEN** a known voltage (e.g., 12.00V) is applied to the divider input
- **THEN** the ADC reading SHALL convert to 12.00V ± 0.1V after calibration

### Requirement: Periodic voltage sampling

The system SHALL sample the battery voltage at a configurable interval (default: 1 Hz) and store the latest reading.

#### Scenario: Periodic sampling
- **WHEN** the ADC sampler is started with a 1-second interval
- **THEN** the system SHALL read and publish a new voltage value approximately once per second

### Requirement: Low voltage alert

The system SHALL generate an alert when battery voltage drops below 11.5V (engine off) or 13.0V (engine running, indicated by RPM > 0).

#### Scenario: Low battery with engine off
- **WHEN** voltage drops below 11.5V and RPM is 0
- **THEN** the system SHALL set VehicleData.battery_low flag to true

#### Scenario: Alternator failure with engine running
- **WHEN** voltage drops below 13.0V and RPM > 0
- **THEN** the system SHALL set VehicleData.charging_fault flag to true

### Requirement: Voltage divider configuration

The system SHALL use a resistive voltage divider (10kΩ upper, 3.3kΩ lower) to scale the automotive voltage (max 15V) to the ADS1115 input range (max 4.096V at PGA gain 1).

#### Scenario: Divider scaling
- **WHEN** 14.4V is applied to the divider input
- **THEN** the ADC input voltage SHALL be approximately 3.57V (14.4 × 3300/13300)
- **THEN** this SHALL be within the ADS1115 FSR (±4.096V at gain 1)
