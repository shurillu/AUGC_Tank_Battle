#include <Servo.h>

Servo myServo;

void setup()
{
	myServo.attach(D0);
	delay(1000);
	myServo.writeMicroseconds(1000);
	delay(1000);
	myServo.writeMicroseconds(2000);
	delay(1000);
	myServo.writeMicroseconds(1500);
	delay(1000);
}

void loop()
{
}
