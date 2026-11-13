//! RadioKit 2-page cockpit UI definition matching the RadioKit companion app.

use rk_rs::*;

// ── Widget ID Constants ──
// Page 0: Truck
pub const WID_STEERING: u8 = 0;
pub const WID_GAS_PEDAL: u8 = 1;
pub const WID_BRAKE_PEDAL: u8 = 2;
pub const WID_TRUCK_LIGHT: u8 = 3;
pub const WID_START_BUTTON: u8 = 4;
pub const WID_LEFT_INDICATOR: u8 = 5;
pub const WID_RIGHT_INDICATOR: u8 = 6;
pub const WID_AUX_SLIDER: u8 = 7;
pub const WID_HORN_BUTTON: u8 = 8;
pub const WID_GEAR_SWITCH: u8 = 9;

// Page 1: Loco
pub const WID_LOCO_THROTTLE: u8 = 10;
pub const WID_DIR_SWITCH: u8 = 11;
pub const WID_LOCO_LIGHT: u8 = 12;
pub const WID_BELL_BUTTON: u8 = 13;
pub const WID_LOCO_ENGINE: u8 = 14;

// Telemetry
pub const WID_TELEM_BATTERY: u8 = 15;
pub const WID_TELEM_SPEED: u8 = 16;

pub const COCKPIT_WIDGET_COUNT: usize = 17;

/// Construct the 2-page RadioKit layout descriptors for RC Engine.
pub fn build_cockpit_descriptors<'a>(storage: &mut [WidgetDescriptor<'a>; RK_MAX_WIDGETS]) -> usize {
    let mut i = 0;

    // ── Page 0: Truck ──
    // Steering Wheel (Knob)
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::Knob,
        widget_id: WID_STEERING,
        x: 163,
        y: 54,
        width: 0,
        height: 64,
        rotation: 0,
        style: WidgetStyle::Primary,
        variant: 1, // Steering wheel variant
        page: 0,
        label: "steering_wheel",
        icon: None,
        on_text: None,
        off_text: None,
        content: None,
        extra: None,
        label_hidden: true,
        hidden: false,
    };
    i += 1;

    // Gas Pedal (Slider)
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::Slider,
        widget_id: WID_GAS_PEDAL,
        x: 49,
        y: 55,
        width: 20,
        height: 56,
        rotation: 0,
        style: WidgetStyle::Primary,
        variant: pack_variant(CenteringMode::Min, 0, true),
        page: 0,
        label: "gas_pedal",
        icon: None,
        on_text: None,
        off_text: None,
        content: None,
        extra: None,
        label_hidden: true,
        hidden: false,
    };
    i += 1;

    // Brake Pedal (Slider)
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::Slider,
        widget_id: WID_BRAKE_PEDAL,
        x: 21,
        y: 65,
        width: 32,
        height: 33,
        rotation: 0,
        style: WidgetStyle::Primary,
        variant: pack_variant(CenteringMode::Min, 0, true),
        page: 0,
        label: "brake_pedal",
        icon: None,
        on_text: None,
        off_text: None,
        content: None,
        extra: None,
        label_hidden: true,
        hidden: false,
    };
    i += 1;

    // Truck Light (MultipleSelect)
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::Multiple,
        widget_id: WID_TRUCK_LIGHT,
        x: 98,
        y: 87,
        width: 100,
        height: 27,
        rotation: 0,
        style: WidgetStyle::Primary,
        variant: 0,
        page: 0,
        label: "truck_light",
        icon: None,
        on_text: None,
        off_text: None,
        content: None,
        extra: None,
        label_hidden: true,
        hidden: false,
    };
    i += 1;

    // Engine Start Button (ToggleButton)
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::ToggleButton,
        widget_id: WID_START_BUTTON,
        x: 98,
        y: 56,
        width: 0,
        height: 20,
        rotation: 0,
        style: WidgetStyle::Success,
        variant: 0,
        page: 0,
        label: "start_button",
        icon: Some("power"),
        on_text: Some("START"),
        off_text: Some("STOP"),
        content: None,
        extra: None,
        label_hidden: false,
        hidden: false,
    };
    i += 1;

    // Left Indicator (ToggleButton)
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::ToggleButton,
        widget_id: WID_LEFT_INDICATOR,
        x: 136,
        y: 16,
        width: 0,
        height: 13,
        rotation: 0,
        style: WidgetStyle::Warning,
        variant: 0,
        page: 0,
        label: "left_indicator",
        icon: Some("arrow-left"),
        on_text: None,
        off_text: None,
        content: None,
        extra: None,
        label_hidden: true,
        hidden: false,
    };
    i += 1;

    // Right Indicator (ToggleButton)
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::ToggleButton,
        widget_id: WID_RIGHT_INDICATOR,
        x: 190,
        y: 16,
        width: 0,
        height: 13,
        rotation: 0,
        style: WidgetStyle::Warning,
        variant: 0,
        page: 0,
        label: "right_indicator",
        icon: Some("arrow-right"),
        on_text: None,
        off_text: None,
        content: None,
        extra: None,
        label_hidden: true,
        hidden: false,
    };
    i += 1;

    // Aux Slider
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::Slider,
        widget_id: WID_AUX_SLIDER,
        x: 121,
        y: 55,
        width: 12,
        height: 40,
        rotation: 0,
        style: WidgetStyle::Primary,
        variant: 0,
        page: 0,
        label: "aux_slider",
        icon: None,
        on_text: None,
        off_text: None,
        content: None,
        extra: None,
        label_hidden: true,
        hidden: false,
    };
    i += 1;

    // Horn Button (PushButton)
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::PushButton,
        widget_id: WID_HORN_BUTTON,
        x: 163,
        y: 15,
        width: 0,
        height: 12,
        rotation: 0,
        style: WidgetStyle::Primary,
        variant: 0,
        page: 0,
        label: "horn_button",
        icon: Some("volume-2"),
        on_text: None,
        off_text: None,
        content: None,
        extra: None,
        label_hidden: true,
        hidden: false,
    };
    i += 1;

    // Gear Switch (MultipleButton)
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::Multiple,
        widget_id: WID_GEAR_SWITCH,
        x: 75,
        y: 56,
        width: 18,
        height: 40,
        rotation: 0,
        style: WidgetStyle::Primary,
        variant: 0,
        page: 0,
        label: "gear_switch",
        icon: None,
        on_text: None,
        off_text: None,
        content: None,
        extra: None,
        label_hidden: true,
        hidden: false,
    };
    i += 1;

    // ── Page 1: Loco ──
    // Throttle Slider
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::Slider,
        widget_id: WID_LOCO_THROTTLE,
        x: 67,
        y: 105,
        width: 20,
        height: 114,
        rotation: 0,
        style: WidgetStyle::Primary,
        variant: 0,
        page: 1,
        label: "throttle_slider",
        icon: None,
        on_text: None,
        off_text: None,
        content: None,
        extra: None,
        label_hidden: true,
        hidden: false,
    };
    i += 1;

    // Reverser Direction Switch (SlideSwitch)
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::SlideSwitch,
        widget_id: WID_DIR_SWITCH,
        x: 67,
        y: 173,
        width: 32,
        height: 18,
        rotation: 0,
        style: WidgetStyle::Primary,
        variant: 0,
        page: 1,
        label: "dir_switch",
        icon: None,
        on_text: Some("FWD"),
        off_text: Some("REV"),
        content: None,
        extra: None,
        label_hidden: true,
        hidden: false,
    };
    i += 1;

    // Loco Light (MultipleSelect)
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::Multiple,
        widget_id: WID_LOCO_LIGHT,
        x: 31,
        y: 97,
        width: 24,
        height: 96,
        rotation: 0,
        style: WidgetStyle::Primary,
        variant: 0,
        page: 1,
        label: "loco_light",
        icon: None,
        on_text: None,
        off_text: None,
        content: None,
        extra: None,
        label_hidden: true,
        hidden: false,
    };
    i += 1;

    // Bell Button (PushButton)
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::PushButton,
        widget_id: WID_BELL_BUTTON,
        x: 31,
        y: 172,
        width: 0,
        height: 20,
        rotation: 0,
        style: WidgetStyle::Primary,
        variant: 0,
        page: 1,
        label: "bell_button",
        icon: Some("bell"),
        on_text: None,
        off_text: None,
        content: None,
        extra: None,
        label_hidden: true,
        hidden: false,
    };
    i += 1;

    // Engine Button (ToggleButton)
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::ToggleButton,
        widget_id: WID_LOCO_ENGINE,
        x: 67,
        y: 41,
        width: 0,
        height: 20,
        rotation: 0,
        style: WidgetStyle::Success,
        variant: 0,
        page: 1,
        label: "engine_button",
        icon: Some("power"),
        on_text: Some("ON"),
        off_text: Some("OFF"),
        content: None,
        extra: None,
        label_hidden: false,
        hidden: false,
    };
    i += 1;

    // ── Telemetry Widgets ──
    storage[i] = WidgetDescriptor {
        type_id: WidgetType::Telemetry,
        widget_id: WID_TELEM_BATTERY,
        x: 0,
        y: 0,
        width: 0,
        height: 0,
        rotation: 0,
        style: WidgetStyle::Primary,
        variant: 0,
        page: 0,
        label: "Battery",
        icon: Some("battery"),
        on_text: None,
        off_text: None,
        content: Some("8.2V"),
        extra: None,
        label_hidden: false,
        hidden: false,
    };
    i += 1;

    storage[i] = WidgetDescriptor {
        type_id: WidgetType::Telemetry,
        widget_id: WID_TELEM_SPEED,
        x: 0,
        y: 0,
        width: 0,
        height: 0,
        rotation: 0,
        style: WidgetStyle::Primary,
        variant: 0,
        page: 0,
        label: "Speed",
        icon: Some("gauge"),
        on_text: None,
        off_text: None,
        content: Some("0 km/h"),
        extra: None,
        label_hidden: false,
        hidden: false,
    };
    i += 1;

    i
}
