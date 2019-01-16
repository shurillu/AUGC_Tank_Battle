#pragma once

#define CARRIER_FREQUENCY 38000 // hz
#define BIT_TIME          1     // ms
#define NO_VALID_DATA     -1    // no data in the receive buffer
class CIR
{
public:
	CIR(uint8_t rxPin, uint8_t txPin);
	~CIR();

	bool sendByte(uint8_t data);
	bool isSendingData(void);

	bool    available(void);
	int16_t receiveByte(void);
	bool    isReceivingData(void);

private:
	uint8_t  m_txPin;
	uint8_t  m_rxPin;
	int16_t  m_receivedData;

};

