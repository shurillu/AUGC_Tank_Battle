#define BLYNK_PRINT   Serial          // must be before #include <BlynkSimpleEsp8266.h>

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Servo.h>
#include "CIR.h"
#include "credentials.h"              // modify here your credentials like:
                                      // - Wifi SSID and password
                                      // - Blynk auth token

#define TURRET_CENTER 98              // the turret center position (degree)
#define PWM_DEADBAND  300             // PWM motor threshold (below this value the motor is stopped)
#define ADC_BATTERY_COEFFICENT 1000.0 // ADC correction factor... depending on the voltage divider
#define BATTERY_VOLTAGE (4.2 / 1024.0) * ADC_BATTERY_COEFFICENT // mv
#define RX_PIN D6                     // IR rx pin
#define TX_PIN D5                     // IR tx pin
#define TURRET_PIN D8                 // servo turret pin
#define MY_ID 0x53                    // one byte tank ID (this code is transmitted when the fire button is pressed)

// motors pin. These values can't be modified (NodeMCU motor shield)
#define L_MOTOR_PWM_PIN D1
#define R_MOTOR_PWM_PIN D2
#define L_MOTOR_DIR_PIN D3
#define R_MOTOR_DIR_PIN D4

#define VIRTUAL_VOLTAGE  V0           // voltage virtual pin. This value is written by the tank to the app
                                      //    Range: [0..4200]

#define VIRTUAL_JOYSTICK V1           // joystick virtual pin. This value is written by the app to the tank 
                                      // It's a combined value:
                                      //    param[0] -> x value. Range: [-1023..+1023] 
                                      //    param[1] -> y value. Range: [-1023..+1023]

#define VIRTUAL_TURRET   V2           // Turret virtual pin. This value is written by the app (turret movement)
                                      // and by the tank (initial setup)
                                      //    Range: [0..180]

#define VIRTUAL_FIRE_BTN V3           // fire push button virtual pin. This value is written by the app to the tank
                                      //    Range: 0 (not pressed), 1 (pressed)

#define VIRTUAL_TERMINAL V5           // terminal virtual pin. This value is written by the tank to the app
                                      // used as log terminal

#define VIRTUAL_ACCELEROMETER V6      // accelerometer virtual pin [FUTURE ADD ONS]

ADC_MODE(ADC_TOUT)                    // NodeMCU ADC initialization: external pin reading values enabled

WidgetTerminal terminal(VIRTUAL_TERMINAL); // for the terminal log
Servo turret;                              // for the servo turret
CIR myIR(RX_PIN, TX_PIN);                  // simple, non locking  IR transceiver library

// Moving, joystick callback. Called every time the joystick values change
BLYNK_WRITE(VIRTUAL_JOYSTICK) {
	// read the joystick position
	int joyX = param[0].asInt();
	int joyY = param[1].asInt();

	int lMotorPWM, rMotorPWM;
	int lMotorDir, rMotorDir;

	// deadband threshold filtering
	if (abs(joyY) <= PWM_DEADBAND)
		joyY = 0;
	if (abs(joyX) <= PWM_DEADBAND)
		joyX = 0;

	// calculate motor PWM output (signed)
	lMotorPWM = -joyX + joyY;
	rMotorPWM = joyX + joyY;

	// calculate motor PWM module and direction... left motor
	if (lMotorPWM < 0) {
		lMotorPWM = -lMotorPWM;
		lMotorDir = HIGH;
	}
	else
		lMotorDir = LOW;

	// calculate motor PWM module and direction... right motor
	if (rMotorPWM < 0) {
		rMotorPWM = -rMotorPWM;
		rMotorDir = LOW;
	}
	else
		rMotorDir = HIGH;

	// PWM cap (PWM duty cycle must be 1023 (100%) or lesser)
	if (lMotorPWM > 1023) lMotorPWM = 1023;
	if (rMotorPWM > 1023) rMotorPWM = 1023;

	// write data to the motors pin
	analogWrite(L_MOTOR_PWM_PIN, lMotorPWM);
	analogWrite(R_MOTOR_PWM_PIN, rMotorPWM);
	digitalWrite(L_MOTOR_DIR_PIN, lMotorDir);
	digitalWrite(R_MOTOR_DIR_PIN, rMotorDir);
}

//turret callback. Called every time the turret values (position) change
BLYNK_WRITE(VIRTUAL_TURRET) {
	// read the turret slider position
	int value = param.asInt();

	// filter the value [0..180]. May be not necessary
	if (value < 0) value = 0;
	if (value > 180) value = 180;
	// the servo is mounted upside-down -> reverse the value
	value = 180 - value;
	// write the value
	turret.write(value);
}


//fire button callback. Called every time the fire button is pressed
BLYNK_WRITE(VIRTUAL_FIRE_BTN) {
	// read the fire button value
	int value = param.asInt();
	// if the butto is pressed (value = =1) and the tank is not already firing
	if ((value == 1) && (!myIR.isSendingData())) {
		// fire sending my ID
		myIR.sendByte(MY_ID);
	}
}


// various system callbacks (not really useful atm)
BLYNK_CONNECTED() {
	Serial.printf("Connected to server!\n");
}

BLYNK_APP_CONNECTED() {
	Serial.printf("APP Connected\n");
}

BLYNK_APP_DISCONNECTED() {
	Serial.printf("APP Disconnected\n");
}

BLYNK_DISCONNECTED() {
	Serial.printf("Disconnected from server\n");
}

void setup()
{
	// Debug console
	Serial.begin(115200);

	// use only one: you can define the Blynk server by his IP Address...
	Blynk.begin(blynkAuthToken, ssid, pass, IPAddress(192, 168, 1, 180), 8080);
	//...or by his symbolic name
//	Blynk.begin(blynkAuthToken, ssid, pass, "www.MYSERVER.com", 8080);

	// servo turret initialization
	turret.attach(TURRET_PIN);
	turret.write(180 - TURRET_CENTER);
	Blynk.virtualWrite(VIRTUAL_TURRET, (180 - TURRET_CENTER));

	// motor pin initialization
	pinMode(L_MOTOR_PWM_PIN, OUTPUT);
	pinMode(R_MOTOR_PWM_PIN, OUTPUT);
	pinMode(L_MOTOR_DIR_PIN, OUTPUT);
	pinMode(R_MOTOR_DIR_PIN, OUTPUT);

	// ADC initialization (for battery voltage reading)
	pinMode(A0, INPUT);

	// log terminal initialization
	terminal.clear();
	terminal.printf("Tank ready!\n");
	terminal.flush();
}

void loop()
{
	Blynk.run(); // Blynk server synchronization

	int voltage = analogRead(A0);                 // read battery voltage [0..1023]
	voltage = (float)voltage * BATTERY_VOLTAGE;   // convert it in mV
	Blynk.virtualWrite(VIRTUAL_VOLTAGE, voltage); // send it to the server -> app

	// if I have received an IR valid packet...
	if (myIR.available()) {
		// print it in the terminal log (with a timing reference) and flush the data
		terminal.printf("[%07lu] HIT by %c\n", millis() / 100, myIR.receiveByte());
		terminal.flush();
	}
}

