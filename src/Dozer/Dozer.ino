#include <Bluetooth.h>
#include <Mecanum.h>
#include <Report.h>
#include <BlackLineSensor.h>
#include <HCSR04.h>

#define loopTime 40
#define debugMode true // Pass to false for production

//                          left                                   right
//                  top              bottom                top              bottom
Mecanum mecanum(22, 23, 2, 1,   24, 25, 3, 1, 6,       26, 27, 4, 1,   28, 29, 5, 1, 7);
//            in1,in2,pwm,offs in1,in2,pwm,offs,stby  in1,in2,pwm,offs in1,in2,pwm,offs,stby

#include "ExtraMeca.h"
Mecaside left(Left);
Mecaside right(Right);
Sideway sideway;
Diagonal diagonal;
Bluetooth bluetooth(&Serial1);
Report report(&Serial, debugMode, 100);
BlackLineSensor blackLine(A0, A1, A2);
//                echo trigger
HCSR04 backDistance(2, 3);

#include "AutoPilot.h"
AutoPilot autoPilot;

void setup ()
{
  // Serial setup //
  {
    Serial1.begin(9600);
    Serial.begin(9600);
    Serial.println("Serial communication's on...");
    Serial.println("Bluetooth communication's on...");
  }
  // Stop the robot  //
  {
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
      //serializePrettyJson(bluetooth.json, Serial);
      // Keypad //
      {
        Serial.println(bluetooth.json["keypad"].as<int>());
        switch (bluetooth.json["keypad"].as<int>())
        {
          case 4:
            autoPilot.line.find.left();
            Serial.println("find left");
            break;
          case 5:
            autoPilot.line.follow.forward();
            break;
          case 6:
            autoPilot.line.find.right();
            break;
          case 10:
            autoPilot.drift();
            break;
          case 11:
            stop();
            break;
          case 12:
            autoPilot.winDance();
            Serial.println("winDance");
            break;
          default:
            Serial.println("nothing");
        }
      }
      // Motors //
      {
        int value;
        // Left Y //
        {
          value = bluetooth.json["joysticks"]["left"]["y"];
          switch (value) {
            case -2:
              left.forward(1023);
              break;
            case -3:
              left.backward(1023);
              break;
            case -1:
              left.stop();
              break;
            default:
              if (value > 512)
                left.forward(value);
              else if (value < 512)
                left.backward(value);
          }
        }
        // Right Y //
        {
          value = bluetooth.json["joysticks"]["right"]["y"];
          switch (value) {
            case -2:
              right.forward(1023);
              break;
            case -3:
              right.backward(1023);
              break;
            case -1:
              right.stop();
              break;
            default:
              if (value > 512)
                right.forward(value);
              else if (value < 512)
                right.backward(value);
          }
        }
        // Left X //
        {
          value = bluetooth.json["joysticks"]["left"]["x"];
          switch (value) {
            case -2:
              sideway.left(1023);
              break;
            case -3:
              sideway.right(1023);
              break;
            case -1:
              break;
            default:
              if (value > 512)
                sideway.left(value);
              else if (value < 512)
                sideway.right(value);
          }
        }
        // Right X //
        {
          value = bluetooth.json["joysticks"]["right"]["x"];
          switch (value) {
            case -2:
              diagonal.forward(1023);
              break;
            case -3:
              diagonal.backward(1023);
              break;
            case -1:
              break;
            default:
              if (value > 512)
                diagonal.forward(value);
              else if (value < 512)
                diagonal.backward(value);
          }
        }
      }
      /*// Response //
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
    }
  }
  else
  {
    report.ntr++;
    report.prob++;
    bluetooth.empty();
  }
  if (report.prob >= 10)
  {
    stop();
  }
  delay(loopTime);
}

void stop ()
{
  mecanum.stop();
  // Add others actuators
}
