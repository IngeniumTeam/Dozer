#include <Bluetooth.h>
#include <Mecanum.h>
#include <Report.h>
#include <BlackLineSensor.h>
#include <Led.h>
#include <Cherry.h>
#include <Digit.h>
#include <Timino.h>

#define loopTime 20
#define defaultSpeed 230
#define debugMode false
#define tankMode true

#define SERVO_1 13
#define SERVO_2 12
#define SERVO_3 11
#define SERVO_4 10
#define SERVO_5 9
#define SERVO_6 8
#define SERVO_7 7
#define SERVO_8 6

//                           left                              right                    mapping                 //
//                 __________________________        __________________________       ____________              //
//                 top        bottom     stby        top        bottom    stby       from       to              //
//              _________    _________    __      _________    _________    _       _______   ______            //
Mecanum mecanum(35, 34, 4,   39, 38, 2,   25,     36, 37, 3,   32, 33, 5,   7,      0, 1023,  0, defaultSpeed); //
//             in1,in2,pwm  in1,in2,pwm          in1,in2,pwm, in1,in2,pwm           min,max   min,max           //

#include <Mecaside.h>
Mecaside left(Left);
Mecaside right(Right);

Bluetooth bluetooth(&Serial1);
Report report(&Serial, debugMode, 100);

BlackLineSensor blackLine(A0, A1, A2);

LedRGB bluetoothLed(28, 27, 26, true);
LedRGB led2(31, 30, 29, true);
Digit digit(49, 48, 7);

SingleServo barrier(SERVO_1, 90, 0);
SingleServo mandible(SERVO_2, 150, 60);
DoubleServo toCake(SERVO_4, SERVO_3, 90, 0, 0, 90);
SingleServo toBasket(SERVO_5, 10, 45);
SingleServo costume(SERVO_6, 0, 40);
Vacuum vacuum(SERVO_7, SingleServo(SERVO_8, 70, 0), true);
void vacuumOff() {
  vacuum.off();
}
Timeout vacuumTimeout(vacuumOff, 4000, false);
void vacuumSequence () {
  vacuum.on();
  vacuumTimeout.start();
}
Interval vacuumLoop(vacuumSequence, 5500, false);

#include "AutoPilot.h"

int estimation = 60;
bool retract = true;

void setup ()
{
  // Serial setup //
  {
    Serial1.begin(9600);
#if debugMode
    Serial.begin(9600);
    Serial.println("Debug mode is on.");
    Serial.println("Serial communication is on...");
    Serial.println("Bluetooth communication is on...");
#endif
  }
  // Setup and stop the robot  //
  {
    pinMode(50, OUTPUT);
    digitalWrite(50, LOW); // The ground pin of the digit
    digit.display(estimation);
    mandible.setup();
    mandible.open();
    barrier.setup();
    barrier.close();
    toCake.setup();
    toBasket.setup();
    toBasket.close();
    costume.setup();
    costume.close();
    vacuum.setup();
    vacuum.off();
    bluetoothLed.off();
    stop();
#if debugMode
    Serial.println("All systems are running.");
#endif
  }
}

void loop ()
{
  vacuumLoop.loop();
  vacuumTimeout.loop();
  report.print();
  if (bluetooth.receive())
  {
    if (bluetooth.lastError == DeserializationError::Ok)
    {
      report.ok++;
      report.prob = 0;
      bluetoothLed.on(0, 0, 255);
      {
#if debugMode
        Serial.print("estimation: "); Serial.println(bluetooth.json["estimation"].as<int>()); Serial.println();
#endif
        if (bluetooth.json["estimation"].as<int>() != -1 && bluetooth.json["estimation"].as<int>() != estimation) {
          estimation = bluetooth.json["estimation"].as<int>();
          digit.display(estimation);
        }
      }
      // Switch //
      {
#if debugMode
        Serial.print("switch: "); Serial.println(bluetooth.json["switch"].as<bool>()); Serial.println();
#endif
        mandible.move(bluetooth.json["switch"].as<bool>());
      }
      // Keypad //
      {
#if debugMode
        Serial.print("key: "); Serial.println(bluetooth.json["keypad"].as<int>()); Serial.println();
#endif
        switch (bluetooth.json["keypad"].as<int>())
        {
          case 1:
            barrier.toggle();
            break;
          case 2:
            if (retract)
              mandible.servo.write(0);
            else
              mandible.move();
            retract = !retract;
            break;
          case 3:
            toCake.open();
            break;
          case 4:
            if (vacuum.toggle())
            {
              vacuumLoop.start();
              mecanum.setMaxSpeed(60);
              mandible.servo.write(0);
            }
            else
            {
              vacuumLoop.cancel();
              vacuumTimeout.cancel();
              mecanum.setMaxSpeed(defaultSpeed);
              mandible.move();
            }
            break;
          case 5:
            break;
          case 6:
            vacuum.servo.close();
            delay(100);
            toBasket.toggle();
            break;
          case 7:
            barrier.open();
            delay(100);
            autoPilot.putCherriesForward();
            break;
          case 8:
            mandible.servo.write(0);
            barrier.open();
            toCake.openAll();
            mecanum.stop();
            break;
          case 9:
            mandible.open();
            delay(100);
            autoPilot.putCherriesBackward();
            break;
          case 10:
            costume.toggle();
            break;
          case 11:
            stop();
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
#if tankMode
          left.move(bluetooth.json["joysticks"]["left"]["y"].as<int>());
          right.move(bluetooth.json["joysticks"]["right"]["y"].as<int>());
#else
          left.move(bluetooth.json["joysticks"]["left"]["y"].as<int>());
          right.move(bluetooth.json["joysticks"]["left"]["y"].as<int>());
#endif
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
  vacuumLoop.cancel();
  vacuumTimeout.cancel();
  vacuum.off();
}
