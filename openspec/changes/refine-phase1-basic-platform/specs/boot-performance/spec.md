## ADDED Requirements

### Requirement: Cold boot time measurement
The system SHALL provide a reproducible method to measure cold boot time from power-on to UI first-frame, broken down by boot stage: U-Boot, kernel, userspace, and application launch.

#### Scenario: systemd-analyze reports stage times
- **WHEN** the user runs `systemd-analyze` after a cold boot
- **THEN** the output shows separate times for firmware (U-Boot), loader, kernel, initrd (if any), and userspace
- **AND** the total time is within 15 seconds

#### Scenario: systemd-analyze blame identifies slow services
- **WHEN** the user runs `systemd-analyze blame`
- **THEN** services are listed in descending order of initialization time
- **AND** no single service exceeds 3 seconds

### Requirement: Boot time baseline documentation
The project SHALL document the baseline boot time measurements for the Armbian development image, stored in `docs/devlog/boot-baseline.md`.

#### Scenario: Boot baseline documented
- **WHEN** Phase 1 boot time measurement is complete
- **THEN** `docs/devlog/boot-baseline.md` contains:
  - U-Boot SPL time, U-Boot proper time, kernel load + decompress time
  - Time to `init` (PID 1), time to `graphical.target` reached
  - Time to Qt eglfs first frame (manually measured)
  - Total cold-boot-to-UI time

### Requirement: Boot optimization targets defined
The project SHALL define target boot times for the production Buildroot image (Phase 9), with per-stage budget allocation.

#### Scenario: Optimization targets documented
- **WHEN** the boot baseline is measured
- **THEN** the optimization targets document specifies:
  - Target total: 5-8 seconds (Buildroot)
  - U-Boot budget: ≤ 1.5s
  - Kernel budget: ≤ 2.5s
  - Userspace to UI budget: ≤ 4s
  - Per-component optimizations listed (kernel trimming, initramfs, service reduction, eMMC HS400)
