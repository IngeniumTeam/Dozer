#include <Bluetooth.h>
#include <Mecanum.h>
#include <Report.h>
#include <BlackLineSensor.h>
#include <Led.h>
#include <Cherry.h>
#include <Timeout.h>

#define loopTime 20
#define debugMode false

#define SERVO_1 13
#define SERVO_2 12
#define SERVO_3 11
#define SERVO_4 10
#define SERVO_5 9
#define SERVO_6 8
#define SERVO_7 7
#define SERVO_8 6

//                          left                                  right                   mapping
//                   top    bottom                 top             bottom           from      to
Mecanum mecanum(34, 35, 4,   38, 39, 2,  25,     37, 36, 3,   32, 33, 5,  7,      0, 1023,  0, 150);
//            in1,in2,pwm    in1,in2,pwm,stby  in1,in2,pwm,   in1,in2,pwm,stby     min,max   min,max

#include <Mecaside.h>
Mecaside left(Left);
Mecaside right(Right);

Bluetooth bluetooth(&Serial1);
Report report(&Serial, debugMode, 100);

BlackLineSensor blackLine(A0, A1, A2);

LedRGB bluetoothLed(28, 27, 26, true);
LedRGB led2(31, 30, 29, true);

Barrier barrier(SERVO_1);
ToCake toCake(SERVO_2, SERVO_3, 90, 0, 40, 0);
ToBasket toBasket(SERVO_4);
Costume costume(SERVO_5);
Grabber grabber(SERVO_6, SERVO_7, 650, 2600, 400, 1000, 0, 130, 0, 130);

#include "AutoPilot.h"

void setup ()
{
  // Serial setup //
  {
    Serial1.begin(9600);
    Serial.begin(9600);
#if debugMode
    Serial.println("Serial communication's on...");
    Serial.println("Bluetooth communication's on...");
    Serial.println("Debug mode's on...");
#endif
  }
  // Setup and stop the robot  //
  {
    barrier.setup();
    toCake.setup();
    toBasket.setup();
    costume.setup();
    grabber.setup();
    bluetoothLed.off();
    stop();
    // autoPilot.drift(); // To drift // Only for fun // Do not use in tournament
  }
}

void loop ()
{
  report.print();
  if (bluetooth.receive())
  {
    if (bluetooth.lastError == DeserializationError::Ok)
    {
      report.ok++;
      report.prob = 0;
      bluetoothLed.on(0, 0, 255);
      // Switch //
      {
#if debugMode
        Serial.print("switch: "); Serial.println(bluetooth.json["switch"].as<bool>()); Serial.println();
#endif
        barrier.move(bluetooth.json["switch"].as<bool>());
      }
      // Keypad //
      {
#if debugMode
        Serial.print("key: "); Serial.println(bluetooth.json["keypad"].as<int>()); Serial.println();
#endif
        switch (bluetooth.json["keypad"].as<int>())
        {
          case 1:
            grabber.grab();
            break;
          case 2:
            toBasket.toggle();
            break;
          case 3:
            toCake.open();
            break;
          case 4:
            autoPilot.line.find.left();
            break;
          case 5:
            autoPilot.line.follow.forward();
            break;
          case 6:
            autoPilot.line.find.right();
            break;
          case 10:
            costume.deploy();
            break;
          case 11:
            stop();
            break;
          case 12:
            autoPilot.winDance();
            break;
        }
      }
      // Motors //
      {
#if debugMode
        Serial.print("y.l: "); Serial.println(bluetooth.json["joysticks"]["left"]["y"].as<int>());
        Serial.print("y.r: "); Serial.println(bluetooth.json["joysticks"]["right"]["y"].as<int>()); Serial.println();
        Serial.print("x.l: "); Serial.println(bluetooth.json["joysticks"]["left"]["x"].as<int>());
        Serial.print("x.r: "); Serial.println(bluetooth.json["joysticks"]["right"]["x"].as<int>()); Serial.println(); Serial.println();
#endif
        // Y
        {
          left.move(bluetooth.json["joysticks"]["left"]["y"].as<int>());
          right.move(bluetooth.json["joysticks"]["right"]["y"].as<int>());
        }
        // X
        {
          mecanum.sideway(bluetooth.json["joysticks"]["left"]["x"].as<int>());
          mecanum.diagonal(bluetooth.json["joysticks"]["right"]["x"].as<int>(), bluetooth.json["joysticks"]["left"]["y"].as<int>());
        }
      }
      /*  // Response //
        {
        bluetooth.json.clear();
        bluetooth.json["blackLine"]["pattern"] = blackLine.getPattern();
        bluetooth.json["blackLine"]["onTheLine"] = blackLine.lastPattern == Position.Pattern.OnTheLine;
        bluetooth.send();
        }*/
    }
    else
    {
      report.inv++;
      report.prob++;
      bluetooth.empty();
      bluetoothLed.on(255, 0, 0);
    }
  }
  else
  {
    report.ntr++;
    report.prob++;
    bluetooth.empty();
    bluetoothLed.off();
  }
  if (report.prob >= 10)
  {
    stop();
  }
  delay(loopTime);
}

void stop ()
{
#if debugMode
  Serial.println("stop"); Serial.println();
#endif
  mecanum.stop();
}
