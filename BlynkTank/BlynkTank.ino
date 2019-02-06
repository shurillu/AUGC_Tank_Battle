#define BLYNK_PRINT   Serial          // must be before #include <BlynkSimpleEsp8266.h>

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "CTank.h"

// default colors
#define BLYNK_GREEN     "#23C48E"
#define BLYNK_BLUE      "#04C0F8"
#define BLYNK_YELLOW    "#ED9D00"
#define BLYNK_RED       "#D3435C"
#define BLYNK_DARK_BLUE "#5F7CD8"
#define BLYNK_GRAY      "#606060"


// Blynk server connection timeout
#define BLYNK_TIMEOUT    5000         // milliseconds

#define VIRTUAL_VOLTAGE  V0           // voltage virtual pin. This value is written by the tank to the app
                                      //    Range: [0..4200]

#define VIRTUAL_JOYSTICK V1           // joystick virtual pin. This value is written by the app to the tank 
                                      // It's a combined value:
                                      //    param[0] -> x value. Range: [-1023..+1023] 
                                      //    param[1] -> y value. Range: [-1023..+1023]

#define VIRTUAL_TURRET   V2           // Turret virtual pin. This value is written by the app (turret movement)
                                      // and by the tank (initial setup)
                                      //    Range: auto (calculated by the tank settings)

#define VIRTUAL_FIRE_BTN V3           // fire push button virtual pin. This value is written by the app to the tank
                                      //    Range: 0 (not pressed), 1 (pressed)

#define VIRTUAL_REPAIR_BTN V4         // repair push button virtual pin. This value is written by the app to the tank
									  //    Range: 0 (not pressed), 1 (pressed)

#define VIRTUAL_TERMINAL V5           // terminal virtual pin. This value is written by the tank to the app
                                      // used as log terminal

#define VIRTUAL_ACCELEROMETER V6      // accelerometer virtual pin [FUTURE ADD ONS]

#define VIRTUAL_HITPOINT V7           // hit points virtual pin. This value is written by the tank to the app
									  //    Range: auto (calculated by the tank settings)

#define VIRTUAL_AMMO V8               // ammos virtual pin. This value is written by the tank to the app
									  //    Range: auto (calculated by the tank settings)

// virtual pins for settings page
#define VIRTUAL_TURRET_CENTER V10    // central server position
#define VIRTUAL_TURRET_LEFT   V11    // left server position
#define VIRTUAL_TURRET_RIGHT  V12    // right server position
#define VIRTUAL_SAVE_SLIDER   V13    // save method

// Spawn ammo timeout: every SPAWN_AMMO_TIME millseconds spawn a new round up to the maximum ammo capacity
// the timer will reset every time the user shoot so, in order to recharge ammo, the user needs to not shoot
#define SPAWN_AMMO_TIMEOUT        7000  // ms

// Battery voltage threshold. If the battery voltage is below this threshold, all the functionalities are disabled
// (motors, shoot, etc).
#define BATTERY_VOLTAGE_THRESHOLD 3500  // mA


// global varibles ----------------------------------------------------------------------------------------------------
WidgetTerminal terminal(VIRTUAL_TERMINAL); // for the terminal log
BlynkTimer voltageTimer;
BlynkTimer spawnAmmoTimer;
int spawnAmmoTimerID;
bool couldMove;
bool couldRepair;

CTank myTank;
int centerAngle;
int spawnAmmoTime = SPAWN_AMMO_TIMEOUT;  // spawn a round every THIS time. If you shoot, the timer restart


// timer handlers -----------------------------------------------------------------------------------------------------
void voltageTimerEvent(void){
	uint16_t voltage = myTank.getBatteryVoltage();
	Blynk.virtualWrite(VIRTUAL_VOLTAGE, voltage);
	if (voltage < BATTERY_VOLTAGE_THRESHOLD) {
		couldMove = false;
		Blynk.setProperty(VIRTUAL_VOLTAGE, "color", BLYNK_RED);
		terminal.printf("[%07lu] LOW BATTERY %umV!!!\n", millis() / 100, voltage);
		terminal.flush();

	}
	else {
		couldMove = true;
		Blynk.setProperty(VIRTUAL_VOLTAGE, "color", BLYNK_GREEN);
	}
}

void spawnAmmoTimerEvent(void) {
	Blynk.virtualWrite(VIRTUAL_AMMO, myTank.newAmmos());
}

void updateTurretSlider(void) {
	Blynk.setProperty(VIRTUAL_TURRET, "min", myTank.getServoMin_us());
	Blynk.setProperty(VIRTUAL_TURRET, "max", myTank.getServoMax_us());
	Blynk.virtualWrite(VIRTUAL_TURRET, (myTank.getServoMin_us() + myTank.getServoMax_us())/2);
}


void blynkTankInit(void) {
	Blynk.virtualWrite(VIRTUAL_TURRET_CENTER, 1500 - myTank.getServoCenter());
	Blynk.virtualWrite(VIRTUAL_TURRET_LEFT, 2000 - myTank.getServoMax_us());
	Blynk.virtualWrite(VIRTUAL_TURRET_RIGHT, 1000 - myTank.getServoMin_us());
	Blynk.setProperty(VIRTUAL_SAVE_SLIDER, "color", BLYNK_GREEN);

	Blynk.setProperty(VIRTUAL_HITPOINT, "min", 0);
	Blynk.setProperty(VIRTUAL_HITPOINT, "max", myTank.getMaxHitpoint());
	Blynk.virtualWrite(VIRTUAL_HITPOINT, 0);

	Blynk.virtualWrite(VIRTUAL_AMMO, myTank.getAmmo());

	Blynk.setProperty(VIRTUAL_VOLTAGE, "color", BLYNK_GREEN);

	Blynk.setProperty(VIRTUAL_REPAIR_BTN, "offBackColor", BLYNK_GRAY);

	myTank.moveTurret_us(myTank.getServoCenter(), true);
	updateTurretSlider();

	// log terminal initialization
	terminal.clear();
	terminal.printf("Tank ready!\n");
	terminal.flush();
	myTank.shakeTurretAnimation(1);
	couldMove = true;
	couldRepair = false;
}


// callback that manage the software trimmer to center the turret
BLYNK_WRITE(VIRTUAL_TURRET_CENTER) {
	int value = param.asInt(); 
	myTank.moveTurret_us(1500 - value, true);
	myTank.setServoCenter(1500 - value);
	Blynk.setProperty(VIRTUAL_SAVE_SLIDER, "color", BLYNK_RED);
}

// callback that manage the software trimmer to set the leftmost turret position
BLYNK_WRITE(VIRTUAL_TURRET_LEFT) {
	int value = param.asInt();
	myTank.moveTurret_us(2000 - value, true);
	myTank.setServoMax_us(2000 - value);
	Blynk.setProperty(VIRTUAL_SAVE_SLIDER, "color", BLYNK_RED);
}

//callback that manage the software trimmer to set the rightmost turret position
BLYNK_WRITE(VIRTUAL_TURRET_RIGHT) {
	int value = param.asInt();
	myTank.moveTurret_us(1000 - value, true);
	myTank.setServoMin_us(1000 - value);
	Blynk.setProperty(VIRTUAL_SAVE_SLIDER, "color", BLYNK_RED);
}

// callback that manage the gesture to save the settings
BLYNK_WRITE(VIRTUAL_SAVE_SLIDER) {
	int value = param.asInt();
	if (value == 1023) {
		Blynk.virtualWrite(VIRTUAL_SAVE_SLIDER, 0);
		if (myTank.writeTankConfigFile()) {
			Blynk.setProperty(VIRTUAL_SAVE_SLIDER, "color", BLYNK_GREEN);
			updateTurretSlider();
			myTank.shakeTurretAnimation(1);
		}
		else
			myTank.shakeTurretAnimation(3);
	}
}


// Moving, joystick callback. Called every time the joystick values change
BLYNK_WRITE(VIRTUAL_JOYSTICK) {
	// move only if the battery is not depleted
	if (!couldMove)
		return;

	// move only if the tank don't need to be repaired
	if (couldRepair)
		return;

	// read the joystick position
	int joyX = param[0].asInt();
	int joyY = param[1].asInt();
	myTank.moveTank(joyX, joyY);
}

//turret callback. Called every time the turret values (position) change
BLYNK_WRITE(VIRTUAL_TURRET) {
	// move only if the battery is not depleted
	if (!couldMove)
		return;

	// move only if the tank don't need to be repaired
	if (couldRepair)
		return;

	// read the turret slider position
	int value = param.asInt();
	myTank.moveTurret_us(value); // move the turret
}

//fire button callback. Called every time the fire button is pressed
BLYNK_WRITE(VIRTUAL_FIRE_BTN) {
	// fire only if the battery is not depleted
	if (!couldMove)
		return;

	// fire only if the tank don't need to be repaired
	if (couldRepair)
		return;

	// read the fire button value
	int value = param.asInt();

	// if the button is pressed (value == 1)
	if (value == 1) {
		if (myTank.shoot()) {// shoot an ammo
			Blynk.virtualWrite(VIRTUAL_AMMO, myTank.getAmmo());
			spawnAmmoTimer.restartTimer(spawnAmmoTimerID);      // the ammos regeneration works only if the tank don't shoot
		}
	}
}

//repair button callback. Called every time the repair button is pressed
BLYNK_WRITE(VIRTUAL_REPAIR_BTN) {
	// repair only if the repair option is enabled/possible
	if (!couldRepair)
		return;

	int value = param.asInt();
	if (1 == value) {
		int currentDamage = myTank.getMaxHitpoint() - myTank.repairTank();
		Blynk.virtualWrite(VIRTUAL_HITPOINT, currentDamage);
		if (0 == currentDamage) {
			Blynk.setProperty(VIRTUAL_REPAIR_BTN, "offBackColor", BLYNK_GRAY);
			couldRepair = 0;

		}
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
	myTank.wifiConnect();

	while (!Blynk.connected()) {
		char server[40];
		char token[40];
		strcpy(server, myTank.getBlynkServer().c_str());
		strcpy(token, myTank.getBlynkToken().c_str());
		if (myTank.isBlynkKnownByIP()) {
			Serial.printf("Connecting using IP: %s\n", myTank.getBlynkIP().toString().c_str());
			Blynk.config(token, myTank.getBlynkIP(), myTank.getBlynkPort());
		}
		else {
			Serial.printf("Connecting using server: %s\n", myTank.getBlynkServer().c_str());
			Blynk.config(token, server, myTank.getBlynkPort());
		}
		Blynk.connect(BLYNK_TIMEOUT);
		if (!Blynk.connected()) {
			Serial.printf("Unable to connect to %s server. Launching hotspot...\n", myTank.getBlynkServer().c_str());
			myTank.startHotspot();
		}
	}

	blynkTankInit();

	voltageTimer.setInterval(100, voltageTimerEvent);
	spawnAmmoTimerID = spawnAmmoTimer.setInterval(spawnAmmoTime, spawnAmmoTimerEvent);
	myTank.shakeTurretAnimation(1);
}

void loop()
{
	Blynk.run(); // Blynk server synchronization
	voltageTimer.run();
	spawnAmmoTimer.run();

	// if I have received an IR valid packet...
	int hitCode = myTank.getHitCode();
	if (hitCode != -1) {
		int currentDamage = myTank.getMaxHitpoint() - myTank.gotHit();
		Blynk.virtualWrite(VIRTUAL_HITPOINT, currentDamage);
		// print it in the terminal log (with a timing reference) and flush the data
		terminal.printf("[%07lu] HIT by %c\n", millis() / 100, hitCode);
		terminal.flush();
		if (myTank.getMaxHitpoint() == currentDamage) {
			Blynk.setProperty(VIRTUAL_REPAIR_BTN, "offBackColor", BLYNK_GREEN);
			couldRepair = true;
		}
	}
}

