## ADDED Requirements

### Requirement: VehicleData shared structure

The system SHALL provide a thread-safe `VehicleData` structure containing decoded CAN values: engine RPM, vehicle speed, coolant temperature, throttle position, battery voltage, and fuel level.

#### Scenario: Concurrent read/write safety
- **WHEN** one thread writes a new RPM value while another thread reads VehicleData
- **THEN** the reader SHALL see either the old or new value (atomic), never a partial update

#### Scenario: Default values
- **WHEN** VehicleData is first created
- **THEN** all fields SHALL be initialized to 0 or sentinel values indicating "no data"

### Requirement: PT-CAN message decoding

The system SHALL decode E46 PT-CAN messages including engine RPM, vehicle speed, coolant temperature, and throttle position from raw CAN frames.

#### Scenario: Engine RPM decoding
- **WHEN** a CAN frame with the RPM ARBID (to be confirmed via candump) arrives
- **THEN** the system SHALL extract RPM as a 16-bit integer from the correct byte offset
- **THEN** the value SHALL be stored in VehicleData.rpm

#### Scenario: Vehicle speed decoding
- **WHEN** a CAN frame with the speed ARBID arrives
- **THEN** the system SHALL extract speed in km/h from the correct byte offset
- **THEN** the value SHALL be stored in VehicleData.speed_kmh

#### Scenario: Coolant temperature decoding
- **WHEN** a CAN frame with the coolant temperature ARBID arrives
- **THEN** the system SHALL extract temperature in degrees Celsius from the correct byte offset
- **THEN** the value SHALL be stored in VehicleData.coolant_temp_c

#### Scenario: Unknown ARBID handling
- **WHEN** a CAN frame with an unrecognized ARBID arrives
- **THEN** the decoder SHALL silently ignore it without crashing or logging spam

### Requirement: CAN decoder extensibility

The system SHALL support adding new ARBID decoders via a registration pattern without modifying core decoding logic.

#### Scenario: Adding a new decoder
- **WHEN** a new ARBID handler is registered with the decoder
- **THEN** frames matching that ARBID SHALL be dispatched to the new handler
- **THEN** existing handlers SHALL continue to function unchanged

### Requirement: CAN frame timestamps

The system SHALL record the arrival timestamp (monotonic clock) for each decoded CAN frame.

#### Scenario: Timestamp recording
- **WHEN** a CAN frame is decoded
- **THEN** VehicleData SHALL include the monotonic timestamp of the last update for each data field
