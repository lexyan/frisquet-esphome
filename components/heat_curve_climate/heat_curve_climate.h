#pragma once

#include "esphome/components/climate/climate.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace climate {
namespace heat_curve {

class HeatCurveClimate : public Climate, public Component {
 public:
  void update();

  void set_sensor(sensor::Sensor *sensor) { current_sensor_ = sensor; }
  void set_outdoor_sensor(sensor::Sensor *sensor) { outoor_sensor_ = sensor; }
  void set_heat_required(bool value);
  void set_output(output::FloatOutput *output) { output_ = output; }
  void set_heat_factor(float heatfactor) { heat_factor_ = heatfactor; }
  void set_offset(float offset) { offset_ = offset; }
  void set_kp(float kp) { kp_ = kp; }
  void set_ki(float ki) { ki_ = ki; }
  void set_minimum_output(float min) { minimum_output_ = 100 * min; }
  void set_maximum_output(float max) { maximum_output_ = 100 * max; }
  void set_heat_required_output(float heatreq_out) { heat_required_output_ = 100 * heatreq_out; }
  void set_output_calibration_factor(float factor) { output_calibration_factor_ = factor; }
  void set_output_calibration_offset(float offset) { output_calibration_offset_ = offset; }
  void reset_integral_term() { integral_term_ = 0; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }

  void add_temperature_computed_callback(std::function<void()> &&callback) {
    water_temp_computed_callback_.add(std::move(callback));
  }

  float get_output_value() const { return output_value_; }
  float get_water_temp() { return water_temp_; }
  float get_proportional_term() { return proportional_term_; }
  float get_integral_term() const { return integral_term_; }
  float get_error() { return error_; }
  float get_delta() { return delta_; }
  float get_heat_factor() { return heat_factor_; }
  float get_offset() { return offset_; }
  float get_kp() { return kp_; }
  float get_ki() { return ki_; }

  void set_default_target_temperature(float default_target_temperature) {
    default_target_temperature_ = default_target_temperature;
  }

 protected:
  void control(const climate::ClimateCall &call) override;
  climate::ClimateTraits traits() override;

  void write_output_(float value);

  float calculate_relative_time_();
  bool in_deadband();
  void calculate_proportional_term_();
  void calculate_integral_term_();

  // Parameters & inputs
  float heat_factor_ = 1.7;
  float dt_;
  float kp_ = 0;
  float ki_ = 0;
  float offset_ = 20;
  float outdoor_temp_ = NAN;
  float output_calibration_factor_ = 1;
  float output_calibration_offset_ = 0;
  float minimum_output_ = 10;
  float maximum_output_ = 100;
  bool heat_required_ = false;
  float heat_required_output_ = minimum_output_;

  uint32_t last_time_ = 0;

  // Sensors
  sensor::Sensor *current_sensor_{nullptr};
  sensor::Sensor *outoor_sensor_{nullptr};
  output::FloatOutput *output_{nullptr};

  CallbackManager<void()> water_temp_computed_callback_;
  float default_target_temperature_;
  bool do_publish_ = false;

  // Results
  float output_value_ = 0;
  float water_temp_ = 20;
  float delta_ = NAN;
  float error_ = NAN;
  float integral_term_ = 0;
  float proportional_term_ = 0;
};

template <typename... Ts>
class SetControlParametersAction : public Action<Ts...> {
 public:
  SetControlParametersAction(HeatCurveClimate *parent) : parent_(parent) {}

  void play(Ts... x) {
    auto heat_factor = this->heat_factor_.value(x...);
    auto offset = this->offset_.value(x...);
    auto kp = this->kp_.value(x...);
    auto ki = this->ki_.value(x...);

    this->parent_->set_heat_factor(heat_factor);
    this->parent_->set_offset(offset);
    this->parent_->set_kp(kp);
    this->parent_->set_ki(ki);
    this->parent_->dump_config();
    this->parent_->update();
  }

 protected:
  TEMPLATABLE_VALUE(float, heat_factor)
  TEMPLATABLE_VALUE(float, offset)
  TEMPLATABLE_VALUE(float, kp)
  TEMPLATABLE_VALUE(float, ki)

  HeatCurveClimate *parent_;
};

template <typename... Ts>
class PIDResetIntegralTermAction : public Action<Ts...> {
 public:
  PIDResetIntegralTermAction(HeatCurveClimate *parent) : parent_(parent) {}

  void play(Ts... x) { this->parent_->reset_integral_term(); }

 protected:
  HeatCurveClimate *parent_;
};

}  // namespace heat_curve
}  // namespace climate
}  // namespace esphome
