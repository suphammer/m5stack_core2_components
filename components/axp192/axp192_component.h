#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/i2c/i2c.h"
#include "axp192.h"

namespace esphome {
namespace axp192 {
enum monitor_type : uint8_t {
  MONITOR_PLUGGED,
  MONITOR_CHARGING,
  MONITOR_OVERTEMP,
  MONITOR_LOWBAT,
  MONITOR_CRITBAT,
  MONITOR_CHARGED,
};
class axp192_binary_sensor : public binary_sensor::BinarySensor, public Component {
 public:
  void set_type(const monitor_type &m) { monitor_ = m; }
  void update(uint8_t input, uint8_t power, uint32_t irq);
  std::string device_class() override;
  bool is_status_binary_sensor() const override { return true; };
  float get_setup_priority() const override { return setup_priority::DATA; };

 private:
  monitor_type monitor_ = monitor_type::MONITOR_PLUGGED;
  bool last_state_ = false;
};

class axp192_sensor : public sensor::Sensor, public Component {
  float get_setup_priority() const override { return setup_priority::DATA; };
};

class axp192_component;
class axp192_backlight : public Component, public light::LightOutput {
 public:
  void set_axp_parent(axp192_component *parent) { parent_ = parent; }
  float get_setup_priority() const override { return setup_priority::HARDWARE; };
  light::LightTraits get_traits() override;
  void write_state(light::LightState *state) override;

 private:
  axp192_component *parent_{nullptr};
};

class axp192_component : public PollingComponent, public i2c::I2CDevice {
 public:
  void set_batterylevel_sensor(axp192_sensor *batterylevel_sensor) { batterylevel_sensor_ = batterylevel_sensor; }
  void set_brightness(float brightness) { brightness_ = brightness; }
  void register_monitor(axp192_binary_sensor *monitor) { monitors_.push_back(monitor); }

  // ========== INTERNAL METHODS ==========
  // (In most use cases you won't need these)
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; };
  void update() override;
  void loop() override;

 protected:
  axp192_sensor *batterylevel_sensor_ = nullptr;
  std::vector<axp192_binary_sensor *> monitors_;
  float brightness_{1.0f};
  float curr_brightness_{-1.0f};

 private:
  AXP192 *axp_ = nullptr;
};

}  // namespace axp192
}  // namespace esphome
