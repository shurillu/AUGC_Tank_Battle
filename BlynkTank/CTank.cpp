#include "CTank.h"
#include "FS.h"

ADC_MODE(ADC_TOUT) // NodeMCU ADC initialization: external pin reading values enabled

// motors pin. These values can't be modified (NodeMCU motor shield)
#define L_MOTOR_PWM_PIN D1
#define R_MOTOR_PWM_PIN D2
#define L_MOTOR_DIR_PIN D3
#define R_MOTOR_DIR_PIN D4

#define TX_PIN          D5 // IR tx pin
#define RX_PIN          D6 // IR rx pin
#define TURRET_PIN      D8 // servo turret pin

#define TURRET_CENTER 98              // the turret center position (degree)
#define PWM_DEADBAND  300             // PWM motor threshold (below this value the motor is stopped)
#define ADC_BATTERY_COEFFICENT 1000.0 // ADC correction factor... depending on the voltage divider
#define BATTERY_VOLTAGE (4.2 / 1024.0) * ADC_BATTERY_COEFFICENT // mv
#define MY_ID 0x53                    // one byte tank ID (this code is transmitted when the fire button is pressed)

// network defaults
#define DEFAULT_SSID         "mySSID"
#define DEFAULT_PSWD         "myPassword"
#define DEFAULT_HOTSPOT_SSID "AUGCMyTank"
#define DEFAULT_HOTSPOT_PSWD ""
#define DEFAULT_BLYNK_SERVER "blynk-cloud.com"
#define DEFAULT_BLYNK_PORT   "80"
#define DEFAULT_BLYNK_TOKEN  "myBlynkToken"

// servo defaults
#define DEFAULT_SERVO_CENTER 1500
#define DEFAULT_SERVO_MIN_US 1000
#define DEFAULT_SERVO_MAX_US 2000

#define SERVO_RANGE 500

// time on startup while finding IR carrier to load configuration hotspot.
// if you want to load the configuration hotspot, simply put the cannon near a wall
#define CONFIG_TIMEOUT     7000  // milliseconds

// how much time the tank must sense the IR carrier for starting the hotspot
#define HOTSPOT_TIMEOUT	   2000  // milliseconds

// how many seconds should try to connect to the wifi network
#define WIFI_TIMEOUT       10    // seconds

#define DEFAULT_MAX_HITPOINT       200
#define DEFAULT_AMMO_RECHARGE_TIME 1500 // milliseconds
#define DEFAULT_AMMO_DAMAGE        40
#define DEFAULT_MAX_AMMO           20
#define DEFAULT_REPAIR_VALUE       5
#define DEFAULT_AMMO_SPAWN_TIME    7000  // milliseconds


#define NETWORK_CONFIG_FILE "/network.cfg"
#define TANK_CONFIG_FILE    "/tank.cfg"

// tags for network configuration file
#define VERSION_TAG      "Version = "
#define WIFI_SSID_TAG    "WiFiSSID = "
#define WIFI_PSWD_TAG    "WiFiPassword = "
#define HS_SSID_TAG      "HotspotSSID = "
#define HS_PSWD_TAG      "HotspotPassword = "
#define BLYNK_SERVER_TAG "BlynkServer = "
#define BLYNK_PORT_TAG   "BlynkPort = "
#define BLYNK_TOKEN_TAG  "BlynkToken = "

// tags fot tank configuration file
#define SERVO_CENTER_TAG "ServoCenter = "
#define SERVO_MIN_US_TAG "ServoMin_us = "
#define SERVO_MAX_US_TAG "ServoMax_us = "



// WifiManager callbacks and variables ------------------------------------------------------------------------
bool shouldSaveConfig;

// enter in config mode
void configModeCallback(WiFiManager *myWiFiManager) {
	Serial.println("Entered config mode");
	Serial.println(WiFi.softAPIP());
	Serial.println(myWiFiManager->getConfigPortalSSID());
}

//callback notifying us of the need to save config. Only when the connection is established
void saveConfigCallback() {
	Serial.println("Should save config");
	shouldSaveConfig = true;
}
// ------------------------------------------------------------------------------------------------------------

// ammoReloadTimer callback. Used to simulate the ammo reload time 
void ammoReload(CTank* tank) {
	tank->ammoReloadDone();
	Serial.println("Recharged");
}


void spawnAmmo(CTank* tank) {
	tank->newAmmos(1);
}


CTank::CTank():CTank(false)
{
}

CTank::CTank(bool formatFS)
{
	// IR transceiver object
	m_pIRcom = new CIR(RX_PIN, TX_PIN);

	// servo turret initialization
	m_turret.attach(TURRET_PIN);

	// motor pin initialization
	pinMode(L_MOTOR_PWM_PIN, OUTPUT);
	pinMode(R_MOTOR_PWM_PIN, OUTPUT);
	pinMode(L_MOTOR_DIR_PIN, OUTPUT);
	pinMode(R_MOTOR_DIR_PIN, OUTPUT);

	// ADC initialization (for battery voltage reading)
	pinMode(A0, INPUT);

	initFS(formatFS);
	if (!readNetworkConfigFile())
		setNetworkConfigDefaults();

	if (!readTankConfigFile())
		setTankConfigDefaults();

	m_maxHitPoints     = DEFAULT_MAX_HITPOINT;
	m_hitPoints        = DEFAULT_MAX_HITPOINT;
	m_maxAmmo          = DEFAULT_MAX_AMMO;
	m_ammo             = DEFAULT_MAX_AMMO;
	m_ammoDamage       = DEFAULT_AMMO_DAMAGE;
	m_ammoRechargeTime = DEFAULT_AMMO_RECHARGE_TIME;
	m_repairValue      = DEFAULT_REPAIR_VALUE;
	m_isReloading      = false;
	m_ammoSpawnTime    = DEFAULT_AMMO_SPAWN_TIME;
	m_canRespawnAmmo   = true;
}

CTank::~CTank()
{
	delete m_pIRcom;
}

void CTank::moveTank(int joystickX, int joystickY) {
	int lMotorPWM, rMotorPWM;
	int lMotorDir, rMotorDir;

	// deadband threshold filtering
	if (abs(joystickY) <= PWM_DEADBAND)
		joystickY = 0;
	if (abs(joystickX) <= PWM_DEADBAND)
		joystickX = 0;

	// calculate motor PWM output (signed)
	lMotorPWM = -joystickX + joystickY;
	rMotorPWM = joystickX + joystickY;

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

void CTank::moveTurretDegree(int angle)
{
	// filter the value [0..180]. May be not necessary
	if (angle < 0) angle = 0;
	if (angle > 180) angle = 180;
	// the servo is mounted upside-down -> reverse the value
	angle = 180 - angle;
	// write the value
	m_turret.write(angle);
}

void CTank::moveTurret_us(int us, bool absolute)
{
	if (!absolute) {
		us = m_servoMax_us - (us - m_servoMin_us);
	}
	if (us < 1000 - SERVO_RANGE)
		us = 1000 - SERVO_RANGE;
	if (us > 2000 + SERVO_RANGE)
		us = 2000 + SERVO_RANGE;
	m_turret.writeMicroseconds(us);
}

bool CTank::shoot(void)
{
	if (NULL == m_pIRcom) // check if the IR object is created 
		return(false);
	if (m_isReloading) // check if is recharging ammos
		return(false);
	if (m_pIRcom->isSendingData()) // check if is already sending IR data
		return(false);
	if (0 == m_ammo) // no ammos
		return(false);

	// fire sending my ID
	m_pIRcom->sendByte(MY_ID);
	m_isReloading = true;
	if (m_canRespawnAmmo) {
		m_reloadTimer.once_ms(m_ammoRechargeTime, ammoReload, this);
		if (m_spawnAmmoTimer.active())
			m_spawnAmmoTimer.detach();
		m_spawnAmmoTimer.attach_ms(m_ammoSpawnTime, spawnAmmo, this);
	}
	if (m_ammo > 0)
		m_ammo--;
	shootAnimation();
	return(true);
}

void CTank::shakeTurretAnimation(uint8_t times, bool startFromLeft)
{
	for (uint8_t i = 0; i < times; i++) {
		if (startFromLeft) {
			moveTurretDegree(60);
			delay(300);
			moveTurretDegree(120);
		} else {
			moveTurretDegree(120);
			delay(300);
			moveTurretDegree(60);
		}
		delay(300);
	}
	moveTurret_us(m_servoCenter, true);
}

void CTank::shootAnimation(void)
{
	moveTank(0, -1023);
	delay(50);
	moveTank(0, 1023);
	delay(50);
	moveTank(0, 0);
	delay(50);

}

/*
void CTank::checkConfigPortalRequest(bool force)
{
	bool startWifiManager = force;
	
	if (!force) {
		// enable transmitting IR carrier
		
		Serial.println("Starting configuration time.\n");

		while (!m_pIRcom->transmitCarrier(true))
			delay(1);

		int i = 0;

		uint32_t startTime = millis();
		while ((millis() - startTime < CONFIG_TIMEOUT) && (!startWifiManager)) {
			delay(1);

			startWifiManager = m_pIRcom->detectCarrier();
			if (startWifiManager) {
				i++;
				if (i < 10)
					startWifiManager = false;
			}



//			uint32_t hotspotStartTime = millis();
//			uint32_t hotSpotEndTime = hotspotStartTime;
//			while (m_pIRcom->detectCarrier() && (hotSpotEndTime - hotspotStartTime < HOTSPOT_TIMEOUT)) {
//				hotSpotEndTime = millis();
//				delay(1);
//			}
//			if (hotSpotEndTime - hotspotStartTime >= HOTSPOT_TIMEOUT) {
//				startWifiManager = true;
//				break;
//			}



		}
		m_pIRcom->transmitCarrier(false);
		delay(1);
	}
	if (startWifiManager) {
		Serial.println("Starting hotspot config portal");
		startHotspot();
	}
	else
		Serial.println("Starting tank");
}
*/
bool CTank::wifiConnect(bool autoStartHotspot)
{
	WiFi.begin(m_wifiSSID.c_str(), m_wifiPSW.c_str());  // Connect to the network
	Serial.printf("Connecting to %s", m_wifiSSID.c_str());

	int i = 0;
	while ((WiFi.status() != WL_CONNECTED) && (i <= WIFI_TIMEOUT)) {
		delay(1000);
		Serial.print('.');
		i++;
	}
	Serial.println('\n');

	if (i <= WIFI_TIMEOUT) {
		// connection established
		Serial.println("Connection established!");
		Serial.print("IP address: ");
		Serial.println(WiFi.localIP());
	}
	else {
		// unable to connect -> launch WiFi manager
		if (autoStartHotspot)
			startHotspot();
		else
			return(false);
	}
	return(true);

}

IPAddress CTank::getBlynkIP(void)
{
	if (!isBlynkKnownByIP())
		return IPAddress(0,0,0,0);
	IPAddress ip;
	ip.fromString(m_blynkServer);
	return(ip);
}

String CTank::getBlynkServer(void)
{
	return(m_blynkServer);
}

uint16_t CTank::getBlynkPort(void)
{
	long port = atol(m_blynkPort.c_str());
	if (port < 0)
		port = 0;
	else if (port > 65535)
		port = 0;
	return(port);
}

String CTank::getBlynkToken(void)
{
	return(m_blynkToken);
}

bool CTank::isBlynkKnownByIP(void)
{
	IPAddress ip;
	return (ip.fromString(m_blynkServer));
}

uint16_t CTank::getBatteryVoltage(void)
{
	uint16_t voltage = analogRead(A0);            // read battery voltage [0..1023]
	voltage = (float)voltage * BATTERY_VOLTAGE;   // convert it in mV
	return (voltage);
}

int CTank::getHitCode(void)
{
	if (m_pIRcom->available())
		return(m_pIRcom->receiveByte());
	return(-1);
}

uint16_t CTank::getServoMin_us(void)
{
	return(m_servoMin_us);
}

uint16_t CTank::getServoMax_us(void)
{
	return(m_servoMax_us);
}

uint16_t CTank::getServoCenter(void)
{
	return(m_servoCenter);
}

void CTank::setServoMin_us(uint16_t newValue)
{
	m_servoMin_us = newValue;
}

void CTank::setServoMax_us(uint16_t newValue)
{
	m_servoMax_us = newValue;
}

void CTank::setServoCenter(uint16_t newValue)
{
	m_servoCenter = newValue;
}

uint8_t CTank::getMaxHitpoint(void)
{
	return(m_maxHitPoints);
}

uint8_t CTank::getHitpoint(void)
{
	return(m_hitPoints);
}

uint8_t CTank::getAmmo(void)
{
	return(m_ammo);
}

uint8_t CTank::getMaxAmmo(void)
{
	return(m_maxAmmo);
}

uint8_t CTank::gotHitByDamage(uint8_t damage)
{
	if (((int)m_hitPoints - damage) < 0)
		m_hitPoints = 0;
	else
		m_hitPoints -= damage;
	return(m_hitPoints);
}

uint8_t CTank::gotHit(void)
{
	return(gotHitByDamage(m_ammoDamage));
}

uint8_t CTank::repairTank(void)
{
	if ((m_hitPoints + m_repairValue) > m_maxHitPoints)
		m_hitPoints = m_maxHitPoints;
	else
		m_hitPoints += m_repairValue;
	return(m_hitPoints);
}

uint8_t CTank::newAmmos(uint8_t ammos)
{
	if ((m_ammo + ammos) > m_maxAmmo)
		m_ammo = m_maxAmmo;
	else
		m_ammo += ammos;
	return(m_ammo);
}

bool CTank::initFS(bool formatFS)
{
	// try to initialize the SPI file system
	if (!SPIFFS.begin()) {
		Serial.println("\nSPIFFS initialization failed.");
		return(false);
	}

	if (!SPIFFS.exists(NETWORK_CONFIG_FILE) || formatFS) {
		// no config file present -> format the SPI file system
		if (!SPIFFS.format()) {
			Serial.println("SPIFFS Format error.");
			return(false);
		}
		// create the config file
		if (!writeNetworkConfigFile(true)) {
			Serial.printf("Unable to create %s file.\n", NETWORK_CONFIG_FILE);
			return(false);
		}
	}
	return(true);
}

bool CTank::writeNetworkConfigFile(bool useDefault)
{
	File configFile = SPIFFS.open(NETWORK_CONFIG_FILE, "w");
	if (!configFile) {
		Serial.printf("Unable to create %s file.\n", NETWORK_CONFIG_FILE);
		return(false);
	}

	configFile.printf("%s%s\n", VERSION_TAG, NETWORK_CFG_FILE_VERSION);
	if (useDefault) {
		configFile.printf("%s%s\n", WIFI_SSID_TAG, DEFAULT_SSID);
		configFile.printf("%s%s\n", WIFI_PSWD_TAG, DEFAULT_PSWD);
		configFile.printf("%s%s\n", HS_SSID_TAG, DEFAULT_HOTSPOT_SSID);
		configFile.printf("%s%s\n", HS_PSWD_TAG, DEFAULT_HOTSPOT_PSWD);
		configFile.printf("%s%s\n", BLYNK_SERVER_TAG, DEFAULT_BLYNK_SERVER);
		configFile.printf("%s%s\n", BLYNK_PORT_TAG, DEFAULT_BLYNK_PORT);
		configFile.printf("%s%s\n", BLYNK_TOKEN_TAG, DEFAULT_BLYNK_TOKEN);
	}
	else {
		configFile.printf("%s%s\n", WIFI_SSID_TAG, m_wifiSSID.c_str());
		configFile.printf("%s%s\n", WIFI_PSWD_TAG, m_wifiPSW.c_str());
		configFile.printf("%s%s\n", HS_SSID_TAG, m_hotspotSSID.c_str());
		configFile.printf("%s%s\n", HS_PSWD_TAG, m_hotspotPSW.c_str());
		configFile.printf("%s%s\n", BLYNK_SERVER_TAG, m_blynkServer.c_str());
		configFile.printf("%s%s\n", BLYNK_PORT_TAG, m_blynkPort.c_str());
		configFile.printf("%s%s\n", BLYNK_TOKEN_TAG, m_blynkToken.c_str());
	}
	configFile.close();

	return(true);
}

bool CTank::readNetworkConfigFile(void)
{
	File configFile = SPIFFS.open(NETWORK_CONFIG_FILE, "r");
	if (!configFile) {
		Serial.printf("Unable to open %s file.\n", NETWORK_CONFIG_FILE);
		return(false);
	}

	// read configiguration data
	while (configFile.available()) {
		String data = configFile.readStringUntil('\n');
		if (data.startsWith(VERSION_TAG)) {
			data.replace(VERSION_TAG, "");
			if (data != NETWORK_CFG_FILE_VERSION) {
				Serial.println("Wrong firmware version, loading defaults.");
				// different firmware version -> generate a new default one
				configFile.close();
				writeNetworkConfigFile(true);

				setNetworkConfigDefaults();
				return(true);
			}
		}
		else if (data.startsWith(WIFI_SSID_TAG)) {
			data.replace(WIFI_SSID_TAG, "");
			m_wifiSSID = data;
		}
		else if (data.startsWith(WIFI_PSWD_TAG)) {
			data.replace(WIFI_PSWD_TAG, "");
			m_wifiPSW = data;
		}
		else if (data.startsWith(HS_SSID_TAG)) {
			data.replace(HS_SSID_TAG, "");
			m_hotspotSSID = data;
		}
		else if (data.startsWith(HS_PSWD_TAG)) {
			data.replace(HS_PSWD_TAG, "");
			m_hotspotPSW = data;
		}
		else if (data.startsWith(BLYNK_SERVER_TAG)) {
			data.replace(BLYNK_SERVER_TAG, "");
			m_blynkServer = data;
		}
		else if (data.startsWith(BLYNK_PORT_TAG)) {
			data.replace(BLYNK_PORT_TAG, "");
			m_blynkPort = data;
		}
		else if (data.startsWith(BLYNK_TOKEN_TAG)) {
			data.replace(BLYNK_TOKEN_TAG, "");
			m_blynkToken = data;
		}
	}
	configFile.close();

	return(true);
}

void CTank::startHotspot(void)
{
	shakeTurretAnimation(3);
	WiFiManager wifiManager;
	// wifimanager configuration
	shouldSaveConfig = false;
	wifiManager.setAPCallback(configModeCallback);
	wifiManager.setSaveConfigCallback(saveConfigCallback);
	WiFiManagerParameter customHotspotSSID("HS SSID", "Hotspot SSID", m_hotspotSSID.c_str(), 20);
	wifiManager.addParameter(&customHotspotSSID);
	WiFiManagerParameter customHotspotPSW("HS PSWD", "Hotspot password", m_hotspotPSW.c_str(), 20);
	wifiManager.addParameter(&customHotspotPSW);
	WiFiManagerParameter customBlynkServer("Server", "Blynk Server", m_blynkServer.c_str(), 20);
	wifiManager.addParameter(&customBlynkServer);
	WiFiManagerParameter customBlynkPort("Port", "Blynk Port", m_blynkPort.c_str(), 5);
	wifiManager.addParameter(&customBlynkPort);
	WiFiManagerParameter customBlynkToken("Token", "Blynk Token", m_blynkToken.c_str(), 40);
	wifiManager.addParameter(&customBlynkToken);

#if ENABLE_HOTSPOT_PSW == 0
	wifiManager.startConfigPortal(m_hotspotSSID.c_str());
#else
	wifiManager.startConfigPortal(m_hotspotSSID.c_str(), m_hotspotPSW.c_str());
#endif

	if (shouldSaveConfig) {
		m_wifiSSID    = WiFi.SSID();
		m_wifiPSW     = WiFi.psk();
		m_hotspotSSID = customHotspotSSID.getValue();
		m_hotspotPSW  = customHotspotPSW.getValue();
		m_blynkServer = customBlynkServer.getValue();
		m_blynkPort   = customBlynkPort.getValue();
		m_blynkToken  = customBlynkToken.getValue();

		if (!writeNetworkConfigFile()) {
			Serial.println("Unable to writing config file");
		} else {
			Serial.println("Config file written.");
		}
	}
}

void CTank::ammoReloadDone(void)
{
	m_isReloading = false;
}

bool CTank::writeTankConfigFile(bool useDefault)
{
	File configFile = SPIFFS.open(TANK_CONFIG_FILE, "w");
	if (!configFile) {
		Serial.printf("Unable to create %s file.\n", TANK_CONFIG_FILE);
		return(false);
	}

	configFile.printf("%s%s\n", VERSION_TAG, TANK_CFG_FILE_VERSION);
	if (useDefault) {
		configFile.printf("%s%u\n", SERVO_MIN_US_TAG, DEFAULT_SERVO_MIN_US);
		configFile.printf("%s%u\n", SERVO_MAX_US_TAG, DEFAULT_SERVO_MAX_US);
		configFile.printf("%s%u\n", SERVO_CENTER_TAG, DEFAULT_SERVO_CENTER);
	}
	else {
		configFile.printf("%s%u\n", SERVO_MIN_US_TAG, m_servoMin_us);
		configFile.printf("%s%u\n", SERVO_MAX_US_TAG, m_servoMax_us);
		configFile.printf("%s%u\n", SERVO_CENTER_TAG, m_servoCenter);
	}
	configFile.close();

	return(true);
}

bool CTank::readTankConfigFile(void)
{
	File configFile = SPIFFS.open(TANK_CONFIG_FILE, "r");
	if (!configFile) {
		Serial.printf("Unable to open %s file.\n", TANK_CONFIG_FILE);
		return(false);
	}

	// read configiguration data
	while (configFile.available()) {
		String data = configFile.readStringUntil('\n');
		if (data.startsWith(VERSION_TAG)) {
			data.replace(VERSION_TAG, "");
			if (data != TANK_CFG_FILE_VERSION) {
				Serial.println("Wrong firmware version, loading defaults.");
				// different firmware version -> generate a new default one
				configFile.close();
				writeTankConfigFile(true);
				
				setTankConfigDefaults();
				return(true);
			}
		}
		else if (data.startsWith(SERVO_MIN_US_TAG)) {
			data.replace(SERVO_MIN_US_TAG, "");
			m_servoMin_us = atoi(data.c_str());
		}
		else if (data.startsWith(SERVO_MAX_US_TAG)) {
			data.replace(SERVO_MAX_US_TAG, "");
			m_servoMax_us = atoi(data.c_str());
		}
		else if (data.startsWith(SERVO_CENTER_TAG)) {
			data.replace(SERVO_CENTER_TAG, "");
			m_servoCenter = atoi(data.c_str());
		}
	}
	configFile.close();

	return(true);
}

void CTank::setTankConfigDefaults(void)
{
	m_servoMin_us = DEFAULT_SERVO_MIN_US;
	m_servoMax_us = DEFAULT_SERVO_MAX_US;
	m_servoCenter = DEFAULT_SERVO_CENTER;
}

void CTank::setNetworkConfigDefaults(void)
{
	m_wifiSSID    = DEFAULT_SSID;
	m_wifiPSW     = DEFAULT_PSWD;
	m_hotspotSSID = DEFAULT_HOTSPOT_SSID;
	m_hotspotPSW  = DEFAULT_HOTSPOT_PSWD;
	m_blynkServer = DEFAULT_BLYNK_SERVER;
	m_blynkPort   = DEFAULT_BLYNK_PORT;
	m_blynkToken  = DEFAULT_BLYNK_TOKEN;
}
