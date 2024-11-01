#pragma once

namespace HubPlatform {
namespace TwoGig {
namespace Resources {

#if defined( __HUB_TYPE_H2_SECURE__ )
constexpr char ResetPinGPIO[] = "/sys/class/gpio/gpio66/value";
#else
constexpr char ResetPinGPIO[] = "/dev/null";
#endif

namespace DB {
namespace TableNames {

const char* const Devices = "db.addons.2gig.devices";

}  // namespace TableNames

namespace Fields {

constexpr char PinCodeLength[] = "pin_code_length";
constexpr char PinCodeInputTimeout[] = "pin_code_input_timeout";
constexpr char Type[] = "type";

}  // namespace Fields
}  // namespace DB

namespace Events {

constexpr char AddonReset[] = "addon_reset";
constexpr char ActionStatusUpdated[] = "action_status_updated";

constexpr char NewDeviceDiscovered[] = "new_device_discovered";

constexpr char DeviceAdded[] = "device_added";
constexpr char DeviceRemoved[] = "device_removed";

constexpr char DoorWindowSensorStatusUpdated[] = "door_window_sensor_status_updated";
constexpr char SmokeDetectorStatusUpdated[] = "smoke_detector_status_updated";
constexpr char CoDetectorStatusUpdated[] = "co_detector_status_updated";
constexpr char PirDetectorStatusUpdated[] = "pir_detector_status_updated";
constexpr char GlassBreakStatusUpdated[] = "glass_break_detector_status_updated";
constexpr char FloodTemperatureStatusUpdated[] = "flood_temperature_status_updated";
constexpr char TiltDetectorStatusUpdated[] = "tilt_detector_status_updated";
constexpr char RemoteKeyFobStatusUpdated[] = "remote_keyfob_status_updated";
constexpr char PanicButtonStatusUpdated[] = "panic_button_status_updated";
constexpr char PadStatusUpdated[] = "pad_status_updated";
constexpr char PinCodeEntered[] = "pin_code_entered";
constexpr char RepeaterStatusUpdated[] = "repeater_status_updated";
constexpr char HoneywellSensorStatusUpdated[] = "honeywell_sensor_status_updated";
constexpr char ReachableStatusUpdated[] = "reachable_status_updated";

namespace Parameters {

constexpr char Action[] = "action";
constexpr char Status[] = "status";

constexpr char DeviceType[] = "device_type";
constexpr char Serial[] = "serial";
constexpr char Model[] = "model";
constexpr char LowBattery[] = "low_battery";
constexpr char TamperDetected[] = "tamper_detected";
constexpr char ReedClosed[] = "reed_closed";
constexpr char ExternalContact[] = "external_contact";
constexpr char FreezeAlarm[] = "freeze_alarm";
constexpr char HeatAlarm[] = "heat_alarm";
constexpr char SmokeAlarm[] = "smoke_alarm";
constexpr char CoAlarm[] = "co_alarm";
constexpr char MotionDetected[] = "motion_detected";
constexpr char GlassBreakDetected[] = "glass_break_detected";
constexpr char WaterDetected[] = "water_detected";
constexpr char TiltDetected[] = "tilt_detected";
constexpr char Button1Pressed[] = "button1_pressed";
constexpr char Button2Pressed[] = "button2_pressed";
constexpr char Button3Pressed[] = "button3_pressed";
constexpr char Button4Pressed[] = "button4_pressed";
constexpr char ButtonPressed[] = "button_pressed";
constexpr char JamDetected[] = "jam_detected";
constexpr char AcFailed[] = "ac_failed";
constexpr char Loop1[] = "loop1";
constexpr char Loop2[] = "loop2";
constexpr char Loop3[] = "loop3";
constexpr char Pin[] = "pin";
constexpr char PinState[] = "pin_state";
constexpr char Reachable[] = "reachable";

}  // namespace Parameters

}  // namespace Events

constexpr char TxIds[] = "tx_ids";
constexpr char Configuration[] = "configuration";
constexpr char PinLength[] = "pin_length";
constexpr char PinInputTimeout[] = "pin_input_timeout";

}  // namespace Resources
}  // namespace TwoGig
}  // namespace HubPlatform
