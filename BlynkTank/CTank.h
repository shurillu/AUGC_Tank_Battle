#pragma once
#ifndef CTANK_H
#define CTANK_H

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <Arduino.h>
#include <Servo.h>
#include <Ticker.h>
#include <SoftwareSerial.h>
#include "CIR.h"

#//define FIRMWARE_VERSION    "1.0.0" // firmware version
#define NETWORK_CFG_FILE_VERSION "1.0.0" // network config file version
#define TANK_CFG_FILE_VERSION    "1.0.0" // tank config file version

//  enable hotspot password
#define ENABLE_HOTSPOT_PSW 0 // 0 -> password disabled
                             // 1 -> password enabled


class CTank
{
public:
	CTank();
	CTank(bool formatFS);
	~CTank();
	void moveTank(int joystickX, int joystickY);
	void moveTurretDegree(int angle);
	void moveTurret_us(int us, bool absolute = false);
	bool shoot(void);
	bool checkProximity(uint16_t timeout);
	void shakeTurretAnimation(uint8_t times, bool startFromLeft = true);
	void shootAnimation(void);
	void startHotspot(void);
	bool wifiConnect(bool autoStartHotspot = true);
	IPAddress getBlynkIP(void);
	String    getBlynkServer(void);
	uint16_t  getBlynkPort(void);
	String    getBlynkToken(void);
	bool      isBlynkKnownByIP(void);
	uint16_t  getBatteryVoltage(void);
	int       getHitCode(void);
	uint16_t  getServoMin_us(void);
	uint16_t  getServoMax_us(void);
	uint16_t  getServoCenter(void);
	void      setServoMin_us(uint16_t newValue);
	void      setServoMax_us(uint16_t newValue);
	void      setServoCenter(uint16_t newValue);

	uint8_t   getMaxHitpoint(void);
	uint8_t   getHitpoint(void);
	uint8_t   getAmmo(void);
	uint8_t   getMaxAmmo(void);
	uint8_t   gotHitByDamage(uint8_t damage);
	uint8_t   gotHit(void);
	uint8_t   repairTank(void);
	uint8_t   newAmmos(uint8_t ammos = 1);

	void ammoReloadDone(void);
	void canRespawnAmmo(bool respawn);
	bool writeTankConfigFile(bool useDefaults = false);

	void playSound(uint16_t soundID, bool loop = false);
	void setVolume(uint8_t volume);
	void printMP3Debug(void);

	//	void checkConfigPortalRequest(bool force = false);

private:
	SoftwareSerial *m_pMP3com;
	Servo    m_turret;
	CIR     *m_pIRcom;
	String   m_wifiSSID,
		     m_wifiPSW,
		     m_hotspotSSID,
		     m_hotspotPSW,
		     m_blynkServer,
		     m_blynkPort,
		     m_blynkToken;
	uint16_t m_servoMin_us, m_servoMax_us, m_servoCenter;

	uint8_t  m_maxHitPoints, m_hitPoints;
	uint8_t  m_ammoDamage, m_maxAmmo, m_ammo;
	uint16_t m_ammoRechargeTime, m_ammoSpawnTime;
	uint8_t  m_repairValue;

	uint8_t m_MP3Packet[10];


	Ticker   m_reloadTimer;
	Ticker   m_spawnAmmoTimer;

	bool m_isReloading;
	bool m_canRespawnAmmo;

	bool initFS(bool formatFS = false);
	bool writeNetworkConfigFile(bool useDefaults = false);
	bool readNetworkConfigFile(void);
	bool readTankConfigFile(void);
	void setTankConfigDefaults(void);
	void setNetworkConfigDefaults(void);
	
	void MP3SendCommand(uint8_t command, uint16_t parameter, bool feedback = false);
};

#endif // !CTANK
