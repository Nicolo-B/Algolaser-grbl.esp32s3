#include "I2SOut.h"

#ifndef USE_I2S_OUT

int i2s_out_init(i2s_out_init_t& init_param) { (void)init_param; return 0; }
int i2s_out_init() { return 0; }
uint8_t i2s_out_read(uint8_t pin) { (void)pin; return 0; }
void i2s_out_write(uint8_t pin, uint8_t val) { (void)pin; (void)val; }
uint32_t i2s_out_push_sample(uint32_t usec) { (void)usec; return 0; }
int i2s_out_set_passthrough() { return 0; }
int i2s_out_set_stepping() { return 0; }
void i2s_out_delay() { }
int i2s_out_set_pulse_period(uint32_t usec) { (void)usec; return 0; }
int i2s_out_set_pulse_callback(i2s_out_pulse_func_t func) { (void)func; return 0; }

i2s_out_pulser_status_t IRAM_ATTR i2s_out_get_pulser_status() { return PASSTHROUGH; }
int i2s_out_reset() { return 0; }

#endif // USE_I2S_OUT
