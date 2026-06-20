## ADDED Requirements

### Requirement: DSI display detected by kernel
The Armbian kernel SHALL detect the MIPI DSI display and register it as a DRM connector, visible in `/sys/class/drm/` and `modetest` output.

#### Scenario: DRM connector appears
- **WHEN** the system boots with the 7" DSI LCD connected to the CM4-IO-BASE-B DSI1 connector
- **THEN** `ls /sys/class/drm/` lists at least one `card*-DSI-*` connector
- **AND** `modetest -M rockchip` lists the DSI connector with supported modes

### Requirement: Framebuffer console on DSI display
The boot process SHALL render the kernel boot log and a getty login prompt on the DSI display via the DRM framebuffer console (fbcon).

#### Scenario: Boot messages visible
- **WHEN** the system boots with the DSI LCD connected
- **THEN** kernel boot messages appear on the DSI display within 5 seconds of power-on
- **AND** a login prompt is visible after boot completes

### Requirement: Qt eglfs renders at display native resolution
A Qt application using the eglfs platform plugin SHALL render at the DSI display's native resolution (800×480 for the 7" dev screen) with a refresh rate of at least 60 fps.

#### Scenario: Full-screen colored rect at native resolution
- **WHEN** a Qt eglfs test application renders a full-screen colored rectangle
- **THEN** the rectangle fills the entire display without borders or scaling artifacts
- **AND** the application log reports the surface size as 800×480

#### Scenario: Frame rate verification
- **WHEN** a Qt Quick scene with a rotating element is running
- **THEN** the rendering achieves at least 55 fps sustained over 10 seconds
- **AND** no visual tearing or flickering is observed

### Requirement: GPU acceleration via Panfrost
Qt eglfs SHALL use the Mali-G52 GPU via the Panfrost driver for rendering, confirmed by checking the active EGL vendor and renderer strings.

#### Scenario: EGL reports Mali GPU
- **WHEN** the Qt eglfs application starts and logs `QPlatformIntegration` info
- **THEN** the EGL vendor string contains "Panfrost" or "Mali"
- **AND** the GL renderer string contains "Mali-G52"

### Requirement: Device tree overlay for display timing
The device tree SHALL include correct timing parameters (HFP/HBP/VFP/VBP/HSW/VSW, pixel clock) for the connected DSI panel, applied via device tree overlay or compiled-in node.

#### Scenario: modetest shows correct mode
- **WHEN** the user runs `modetest -M rockchip -c`
- **THEN** the connected display mode matches the panel's datasheet specifications
- **AND** the pixel clock is within 1% of the panel's specified value
