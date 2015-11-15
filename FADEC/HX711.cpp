#include "HX711.h"

HX711::HX711(byte dout, byte pd_sck, byte gain) {
	PD_SCK 	= pd_sck;
	DOUT 	= dout;
	
	pinMode(PD_SCK, OUTPUT);
	pinMode(DOUT, INPUT);

	set_gain(gain);
}

HX711::~HX711() {

}

bool HX711::is_ready() {
	return digitalRead(DOUT) == LOW;
}

void HX711::set_gain(byte gain) {
	switch (gain) {
		case 128:		// channel A, gain factor 128
			GAIN = 1;
			break;
		case 64:		// channel A, gain factor 64
			GAIN = 3;
			break;
		case 32:		// channel B, gain factor 32
			GAIN = 2;
			break;
	}

	digitalWrite(PD_SCK, LOW);
	read();
}

long HX711::read() {
	// wait for the chip to become ready
	while (!is_ready());

	byte data[3];

	// pulse the clock pin 24 times to read the data
	for (byte j = 3; j--;) {
		for (char i = 8; i--;) {
			digitalWrite(PD_SCK, HIGH);
			bitWrite(data[j], i, digitalRead(DOUT));
			digitalWrite(PD_SCK, LOW);
		}
	}

	// set the channel and the gain factor for the next reading using the clock pin
	for (int i = 0; i < GAIN; i++) {
		digitalWrite(PD_SCK, HIGH);
		digitalWrite(PD_SCK, LOW);
	}

	data[2] ^= 0x80;

    m_last = ((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | (uint32_t)data[0];

	return m_last;
}

long HX711::read_average(byte times) {
	long sum = 0;
	for (byte i = 0; i < times; i++) {
		sum += read();
	}
    m_last = sum / times;
    return m_last;
}

long HX711::get_value(byte times) {
	return read_average(times) - OFFSET;
}

float HX711::get_units(byte times) {
	return get_value(times) / SCALE;
}

/**
 * Возврат резульата последних измерений, в масштабе.
 */
float HX711::get_last_units()
{
    return (m_last - OFFSET) / SCALE;
}

long HX711::tare(byte times) {
	long sum = read_average(times);
	set_offset(sum);
    return sum;
}

double HX711::set_scale(float scale) {
	SCALE = scale;
    return SCALE;
}

double HX711::get_scale()
{
    return SCALE;
}

long HX711::set_offset(long offset) {
	OFFSET = offset;
    return OFFSET;
}

long HX711::get_offset()
{
    return OFFSET;
}

void HX711::power_down() {
	digitalWrite(PD_SCK, LOW);
	digitalWrite(PD_SCK, HIGH);	
}

void HX711::power_up() {
	digitalWrite(PD_SCK, LOW);	
}
