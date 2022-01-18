#include "axp192_component.h"
#include "esphome/core/log.h"

namespace detail {
template<typename T> T scale(T input, T input_min, T input_max, T output_min, T output_max) {
  const T dividend = output_max - output_min;
  const T divisor = input_max - input_min;
  const T delta = input - input_min;
  if (divisor == 0) {
    return 0;
  }
  return delta * dividend / divisor + output_min;
}
}  // namespace detail
namespace esphome {
namespace axp192 {
static const char *TAG = "axp192";
static const char *SENSOR_TAG = "axp192.sensor";
static const char *BINARY_TAG = "axp192.binary_sensor";
static const char *LIGHT_TAG = "axp192.light";

std::string axp192_binary_sensor::device_class() {
  if (monitor_ == monitor_type::MONITOR_PLUGGED) {
    return "plug";
  }
  if (monitor_ == monitor_type::MONITOR_CHARGING) {
    return "battery_charging";
  }
  if (monitor_ == monitor_type::MONITOR_OVERTEMP) {
    return "heat";
  }
  if (monitor_ == monitor_type::MONITOR_LOWBAT || monitor_ == monitor_type::MONITOR_CRITBAT) {
    return "battery";
  }
  return "None";
}

void axp192_binary_sensor::update(uint8_t input_status, uint8_t power_status, uint32_t irq_status) {
  bool should_fire = false;
  if (monitor_ == monitor_type::MONITOR_PLUGGED && (input_status & 0b10000000)) {
    should_fire = true;
  }
  if (monitor_ == monitor_type::MONITOR_PLUGGED && (input_status & 0b00100000)) {
    should_fire = true;
  }
  if (monitor_ == monitor_type::MONITOR_CHARGING && (input_status & 0b00000100)) {
    should_fire = true;
  }
  if (monitor_ == monitor_type::MONITOR_CHARGING && (power_status & 0b01000000)) {
    should_fire = true;
  }
  if (monitor_ == monitor_type::MONITOR_OVERTEMP && (power_status & 0b10000000)) {
    should_fire = true;
  }
  // 44:7 - overvoltage 0b10000000 00000000 00000000 00000000
  // 44:4 - overvoltage  0b00010000 00000000 00000000 00000000
  // 44:1 - undervoltage  0b00000010 00000000 00000000 00000000
  // 45:3 - charging  0b00000000 00001000 00000000 00000000
  // 45:0 - undertemp  0b00000000 00000001 00000000 00000000
  // 47:7 - boot  0b00000000 00000000 00000000 10000000
  // 47:6 - shutdown  0b00000000 00000000 00000000 01000000

  // 45:1 - overtemp 0b00000000 00000010 00000000 00000000
  if (monitor_ == monitor_type::MONITOR_OVERTEMP && (irq_status & 0x00020000)) {
    should_fire = true;
  }
  // 46:7 - overtemp 0b00000000 00000000 10000000 00000000
  if (monitor_ == monitor_type::MONITOR_OVERTEMP && (irq_status & 0x00008000)) {
    should_fire = true;
  }
  // 46:4 - bat low  0b00000000 00000000 00010000 00000000
  if (monitor_ == monitor_type::MONITOR_LOWBAT && (irq_status & 0x00001000)) {
    should_fire = true;
  }
  // 47:0 - bat critical low 0b00000000 00000000 00000000 00000001
  if (monitor_ == monitor_type::MONITOR_CRITBAT && (irq_status & 0x00000001)) {
    should_fire = true;
  }
  // 45:2 - finished charging 0b00000000 00000100 00000000 00000000
  if (monitor_ == monitor_type::MONITOR_CHARGED && (irq_status & 0x00040000)) {
    should_fire = true;
  }
  if (last_state_ == should_fire) {
    return;
  }

  ESP_LOGD(BINARY_TAG, "type: %d, input: %s, power: %s, irq: %s", monitor_, uint32_to_string(input_status).c_str(),
           uint32_to_string(power_status).c_str(), uint32_to_string(irq_status).c_str());

  publish_state(should_fire);
  last_state_ = should_fire;
}

light::LightTraits axp192_backlight::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
  return traits;
}

void axp192_backlight::write_state(light::LightState *state) {
  float bright = 0.0;
  state->current_values_as_brightness(&bright);
  if (parent_ != nullptr) {
    ESP_LOGD(TAG, "Brightness=%f", bright);
    parent_->set_brightness(bright);
  }
}

void axp192_component::setup() {
  axp_ = new AXP192();
  axp_->begin();
}

void axp192_component::dump_config() {
  ESP_LOGCONFIG(TAG, "AXP192:");
  LOG_I2C_DEVICE(this);
  LOG_SENSOR("  ", "Battery Level", this->batterylevel_sensor_);
  for (auto monitor : monitors_) {
    LOG_BINARY_SENSOR("  ", "monitor_type", monitor);
  }
}

void axp192_component::update() {
  if (this->batterylevel_sensor_ != nullptr) {
    // To be fixed
    // This is not giving the right value - mostly there to have some sample sensor...
    float vbat = axp_->GetBatVoltage();
    float batterylevel = 100.0 * ((vbat - 3.0) / (4.1 - 3.0));
    ESP_LOGD(TAG, "Got Battery Level=%f (%f)", batterylevel, vbat);
    if (batterylevel > 100.) {
      batterylevel = 100;
    }
    this->batterylevel_sensor_->publish_state(batterylevel);
  }

  auto input_status = axp_->AXPInState();
  auto power_status = axp_->GetBatState();
  bool ac_in = input_status & 0b10000000;
  bool vbus_in = input_status & 0b00100000;
  bool bat_charge = input_status & 0b00000100;
  bool axp_overtemp = power_status & 0b10000000;
  bool charge_req = power_status & 0b01000000;
  bool bat_active = power_status & 0b00001000;

  ESP_LOGD(
      TAG,
      "input: %x, power: %x, ac_in: %d, vbus_in: %d, bat_charge: %d, axp_overtemp: %d, charge_req: %d, bat_active: %d",
      input_status, power_status, ac_in, vbus_in, bat_charge, axp_overtemp, charge_req, bat_active);

  ESP_LOGD(TAG, "Col in: %u Col out: %u Charge: %fmAh cin: %f batc: %f", axp_->GetCoulombchargeData(),
           axp_->GetCoulombdischargeData(), axp_->GetCoulombData(), axp_->GetBatChargeCurrent(), axp_->GetBatCurrent());
}

void axp192_component::loop() {
  auto input_status = axp_->AXPInState();
  auto power_status = axp_->GetBatState();
  auto irq_status = axp_->Read32bit(0x44);
  for (auto monitor : monitors_) {
    monitor->update(input_status, power_status, irq_status);
  }
  if (brightness_ == curr_brightness_) {
    return;
  }
  // because why the f*ck not
  float level = detail::scale(brightness_, 0.0f, 1.0f, 7.0f, 12.0f);
  ESP_LOGD(TAG, "Brightness=%3.3f (Curr: %3.3f) scaled: %3.3f", brightness_, curr_brightness_, level);
  axp_->ScreenBreath(level);
  curr_brightness_ = brightness_;
}
}  // namespace axp192
}  // namespace esphome
