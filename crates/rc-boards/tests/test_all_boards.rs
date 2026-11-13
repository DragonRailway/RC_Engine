//! Multi-board automated verification test suite.
//! Tests pin mapping, voltage divider calibration, and hardware profile consistency
//! across all 5 board profiles.

use bsp_core::Board;
use rc_boards::*;

#[test]
fn test_tracklink_v3_profile() {
    assert_eq!(TrackLinkV3::NAME, "TRACKLINK_V3");
    assert!((TrackLinkV3::VOLTAGE_DIVIDER.ratio() - 2.0).abs() < 1e-4);
    assert_eq!(TrackLinkV3::AUDIO.din, 21);
    assert_eq!(TrackLinkV3::AUDIO.sd, Some(47));
    assert_eq!(TrackLinkV3::POWER.power_enable, Some(33));

    // Verify pin vocabulary
    assert_eq!(TrackLinkV3::resolve_pin("L0"), Some(42));
    assert_eq!(TrackLinkV3::resolve_pin("L1"), Some(11));
    assert_eq!(TrackLinkV3::resolve_pin("L6"), Some(6));
    assert_eq!(TrackLinkV3::resolve_pin("S1"), Some(1));
    assert_eq!(TrackLinkV3::resolve_pin("S2"), Some(2));
    assert_eq!(TrackLinkV3::resolve_pin("S3"), None); // Only 2 servos on TL3
}

#[test]
fn test_rk_ultra_profile() {
    assert_eq!(RkUltra::NAME, "RK_ULTRA");
    // 75k / 10k divider -> ratio = 8.5
    assert!((RkUltra::VOLTAGE_DIVIDER.ratio() - 8.5).abs() < 1e-4);
    assert_eq!(RkUltra::POWER.power_enable, Some(35));
    assert_eq!(RkUltra::POWER.power_button, Some(36));
    assert_eq!(RkUltra::POWER.pump_enable, Some(48));

    // DRV8231A drivers with current sense
    assert_eq!(RkUltra::DRIVER_A.isens, Some(4));
    assert_eq!(RkUltra::DRIVER_B.unwrap().isens, Some(6));

    // Verify 8 servo channels
    assert_eq!(RkUltra::resolve_pin("S1"), Some(8));
    assert_eq!(RkUltra::resolve_pin("S4"), Some(11));
    assert_eq!(RkUltra::resolve_pin("E1"), Some(14));
    assert_eq!(RkUltra::resolve_pin("E4"), Some(39));
    assert_eq!(RkUltra::resolve_pin("A1"), Some(42));
    assert_eq!(RkUltra::resolve_pin("SDA"), Some(37));
}

#[test]
fn test_mikro_v2_profile() {
    assert_eq!(MikroV2::NAME, "MIKRO_V2");
    assert!((MikroV2::VOLTAGE_DIVIDER.ratio() - 2.0).abs() < 1e-4);
    assert_eq!(MikroV2::POWER.power_enable, Some(15));
    assert_eq!(MikroV2::POWER.power_button, Some(14));

    // Independent enable pins on Mikro V2
    assert_eq!(MikroV2::DRIVER_A.enable, Some(17));
    assert_eq!(MikroV2::DRIVER_B.unwrap().enable, Some(11));

    // 8 light channels
    assert_eq!(MikroV2::resolve_pin("L1"), Some(38));
    assert_eq!(MikroV2::resolve_pin("L8"), Some(2));
    assert_eq!(MikroV2::resolve_pin("S4"), Some(8));
}

#[test]
fn test_gtrack_profile() {
    assert_eq!(GTrack::NAME, "GTRACK");
    // 20k / 5.1k divider -> ratio = 4.9215686
    assert!((GTrack::VOLTAGE_DIVIDER.ratio() - 4.9215686).abs() < 1e-4);
    assert_eq!(GTrack::POWER.power_enable, Some(35));
    assert_eq!(GTrack::POWER.servo_enable, Some(44));
    assert_eq!(GTrack::POWER.pump_enable, Some(43));

    // Excavator dual tracks
    assert_eq!(GTrack::DRIVER_A.pwm1, 39);
    assert_eq!(GTrack::DRIVER_B.unwrap().pwm1, 41);

    assert_eq!(GTrack::resolve_pin("L1"), Some(47));
    assert_eq!(GTrack::resolve_pin("A4"), Some(4));
}

#[test]
fn test_tracklink_v2_profile() {
    assert_eq!(TrackLinkV2::NAME, "TRACKLINK_V2");
    assert_eq!(TrackLinkV2::POWER.power_enable, Some(21));
    assert_eq!(TrackLinkV2::DRIVER_B, None); // Single motor driver on V2
}
