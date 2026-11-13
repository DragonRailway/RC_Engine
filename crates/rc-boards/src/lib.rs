//! Umbrella crate re-exporting board support packages and profiles.

#![no_std]

pub use bsp_core::*;
pub use bsp_gtrack::GTrack;
pub use bsp_mikro_v2::MikroV2;
pub use bsp_rk_ultra::RkUltra;
pub use bsp_tracklink_v2::TrackLinkV2;
pub use bsp_tracklink_v3::TrackLinkV3;

/// Legacy BoardProfile structure retained for backwards compatibility.
#[derive(Debug, Clone, Copy)]
pub struct BoardProfile {
    pub name: &'static str,
    pub driver_a: DualPwmPins,
    pub driver_b: Option<DualPwmPins>,
    pub audio: AudioPins,
    pub power: PowerPins,
    pub voltage_divider: ResistorDivider,
}

impl BoardProfile {
    pub const fn from_board<B: Board>() -> Self {
        Self {
            name: B::NAME,
            driver_a: B::DRIVER_A,
            driver_b: B::DRIVER_B,
            audio: B::AUDIO,
            power: B::POWER,
            voltage_divider: B::VOLTAGE_DIVIDER,
        }
    }
}

pub const PROFILE_TRACKLINK_V3: BoardProfile = BoardProfile::from_board::<TrackLinkV3>();
pub const PROFILE_RK_ULTRA: BoardProfile = BoardProfile::from_board::<RkUltra>();
pub const PROFILE_MIKRO_V2: BoardProfile = BoardProfile::from_board::<MikroV2>();
pub const PROFILE_GTRACK: BoardProfile = BoardProfile::from_board::<GTrack>();
pub const PROFILE_TRACKLINK_V2: BoardProfile = BoardProfile::from_board::<TrackLinkV2>();
