// EPOS ARM Cortex Smart Transducer Static Attributes
// File included at the end of include/machine/cortex/transducer.h
// We use this mechanism to avoid always including smart_data.h (which uses periodic thread, which requires specific scheduling policies) when it is not needed by the application

#ifdef __cortex_transducer_h

__BEGIN_SYS

ADC_Sensor::Observed ADC_Sensor::_observed;

Lux_Sensor::Observed Lux_Sensor::_observed;
ADC * Lux_Sensor::_adc;

unsigned int Pressure_Sensor_Keller_Capacitive::_hydro_relay_power;
unsigned int Pressure_Sensor_Keller_Capacitive::_hydro_adc;

unsigned int Water_Level_Sensor_Ultrasonic_Microflex::_hydro_relay_power;
unsigned int Water_Level_Sensor_Ultrasonic_Microflex::_hydro_adc;

unsigned int Water_Turbidity_Sensor_Solar::_hydro_relay_power;
unsigned int Water_Turbidity_Sensor_Solar::_hydro_adc;
unsigned int Water_Turbidity_Sensor_Solar::_hydro_relay_infrared;
unsigned int Water_Turbidity_Sensor_Solar::_hydro_adc_infrared;

Water_Turbidity_Sensor_OBS * Water_Turbidity_Sensor_OBS::_dev[Water_Turbidity_Sensor_OBS::MAX_DEVICES];
unsigned int Water_Turbidity_Sensor_OBS::_hydro_relay_power;
unsigned int Water_Turbidity_Sensor_OBS::_hydro_adc;

Temperature_Sensor::Observed Temperature_Sensor::_observed;

I2C_Temperature_Sensor I2C_Temperature_Transducer::sensor;

Switch_Sensor::Observed Switch_Sensor::_observed;
Switch_Sensor * Switch_Sensor::_dev[Switch_Sensor::MAX_DEVICES];

RFID_Sensor::Observed RFID_Sensor::_observed;
RFID_Sensor * RFID_Sensor::_dev[RFID_Sensor::MAX_DEVICES];

Distance_Sensor * Distance_Sensor::_dev[MAX_DEVICES];
GPIO * Distance_Sensor::_relay;

RS485 * SSB_Substation_Sensor::_dev;
unsigned int SSB_Substation_Sensor::_instances;

//Demonstration_Sensor::Observed Demonstration_Sensor::_observed;
//Smart_Data<Demonstration_Sensor>::Value Demonstration_Sensor::_value[8];

Angular_Rate_Analyzer::Data_Buffer * Angular_Rate_Analyzer::_data_buffer;
LSM330 * Angular_Rate_Analyzer::_imu;
Semaphore * Angular_Rate_Analyzer::_buffer;
Thread * Angular_Rate_Analyzer::_th_publisher;
Thread * Angular_Rate_Analyzer::_th_analyzer;
Observed Angular_Rate_Analyzer::_observed;
bool Angular_Rate_Analyzer::_initialized = false;

__END_SYS

#endif
