#include <Arduino.h>
#include "Ticker.h"
#include "CIR.h"


uint16_t txBuffer;
uint8_t  bitTXed;
Ticker   txTicker;

uint8_t  rxIntPin;
int16_t  rxBuffer;
uint8_t  bitRXed;
Ticker   rxTicker;

void beginReceivingData(void);

void receiveData(void) {
	uint16_t buffer;
	buffer = !digitalRead(rxIntPin);
	rxBuffer += buffer << bitRXed;
	bitRXed++;
	if (10 == bitRXed) {
		rxTicker.detach();
		buffer = rxBuffer & 0xFF;
		uint8_t evenParity = 0;
		for (int i = 0; i < 8; i++) {
			if ((buffer & (1 << i)) != 0)
				evenParity++;
		}
		evenParity = evenParity % 2;
		if (((rxBuffer >> 8) & 0x01) != evenParity) {
			rxBuffer = NO_VALID_DATA;
			attachInterrupt(digitalPinToInterrupt(rxIntPin), beginReceivingData, FALLING);
			return;
		}
		if (((rxBuffer >> 9) & 0x01) != 0x01) {
			attachInterrupt(digitalPinToInterrupt(rxIntPin), beginReceivingData, FALLING);
			rxBuffer = NO_VALID_DATA;
			return;
		}
		rxBuffer = buffer;
	}
}


void beginReceivingData(void) {
	if (NO_VALID_DATA == rxBuffer) {
		detachInterrupt(digitalPinToInterrupt(rxIntPin));
		delayMicroseconds(500);
		bitRXed = 0;
		rxBuffer = 0;
		rxTicker.attach_ms(BIT_TIME, receiveData);
	}
}


void sendData(uint8_t txPin) {
	if (txBuffer & 0x01)
		analogWrite(txPin, (PWMRANGE / DUTY_CYCLE_DIVIDER));
	else
		analogWrite(txPin, 0);

	txBuffer = txBuffer >> 1;
	bitTXed++;
	if (12 == bitTXed) {
		txTicker.detach();
	}
}


CIR::CIR(uint8_t rxPin, uint8_t txPin)
{
	pinMode(txPin, OUTPUT);
	analogWrite(txPin, 0);
	analogWriteFreq(CARRIER_FREQUENCY);
	pinMode(rxPin, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(rxPin), beginReceivingData, FALLING);
	rxIntPin = rxPin;
	m_rxPin = rxPin;
	m_txPin = txPin;
	m_receivedData = NO_VALID_DATA;
	rxBuffer = NO_VALID_DATA;
	m_isTransmittingCarrier = false;
}

CIR::~CIR()
{
	detachInterrupt(digitalPinToInterrupt(m_rxPin));

	if (isSendingData())
		txTicker.detach();
	if (rxTicker.active())
		rxTicker.detach();
}

bool CIR::sendByte(uint8_t data)
{
	uint16_t parity; // even
	// already sending data
	if (isSendingData())
		return false;

	if (m_isTransmittingCarrier)
		transmitCarrier(false);

	parity = 0;
	for (uint8_t i = 0; i < 8; i++) {
		parity += (data >> i) & 0x01;
	}
	parity = parity % 2;
	bitTXed = 0;
	txBuffer = data;
	// Start bit + data + parity (even) + stop bit (1)
	txBuffer = 1 + (txBuffer << 1) + (parity << 9) + 0x0400;
	txTicker.attach_ms(BIT_TIME, sendData, m_txPin);
	return(true);



}

bool CIR::isSendingData(void)
{
	return (txTicker.active());
}

bool CIR::transmitCarrier(bool enable)
{
	if (isSendingData() || isReceivingData())
		return(false);

	if (enable) {
		detachInterrupt(digitalPinToInterrupt(m_rxPin));
		analogWrite(m_txPin, (PWMRANGE / DUTY_CYCLE_DIVIDER));
		m_isTransmittingCarrier = true;
	}
	else {
		analogWrite(m_txPin, 0);
		m_isTransmittingCarrier = false;
		attachInterrupt(digitalPinToInterrupt(m_rxPin), beginReceivingData, FALLING);
	}
	return(true);
}

bool CIR::detectCarrier(void)
{
	uint8_t value = digitalRead(m_rxPin);
	if (value != 0)
		return(false);
	return(true);
}


bool CIR::available(void)
{
//	if (NO_VALID_DATA == m_receivedData)
	if ((NO_VALID_DATA == rxBuffer) || isReceivingData())
		return(false);
	return(true);
}

int16_t CIR::receiveByte(void)
{
	if (isReceivingData())
		return (NO_VALID_DATA);

	int16_t temp = rxBuffer;
	if (NO_VALID_DATA != rxBuffer) {
		rxBuffer = NO_VALID_DATA;
		attachInterrupt(digitalPinToInterrupt(m_rxPin), beginReceivingData, FALLING);
	}
	return(temp);
}

bool CIR::isReceivingData(void)
{
	return (rxTicker.active());
}
