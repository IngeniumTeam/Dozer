#include <Bluetooth.h>
#include <Mecanum.h>
#include <Report.h>
#include <BlackLineSensor.h>
#include <HCSR04.h>

#define loopTime 100

Mecanum mecanum;

struct
{
    void left (int speed)
    {
        mecanum.motors[Left][Top].backward(speed);
        mecanum.motors[Left][Bottom].forward(speed);
        mecanum.motors[Right][Top].backward(speed);
        mecanum.motors[Right][Bottom].forward(speed);
    }
    void right (int speed)
    {
        mecanum.motors[Left][Top].forward(speed);
        mecanum.motors[Left][Bottom].backward(speed);
        mecanum.motors[Right][Top].forward(speed);
        mecanum.motors[Right][Bottom].backward(speed);
    }
} sideway;
struct
{
    void forward (int speed)
    {
        mecanum.motors[Left][Top].forward(speed);
        mecanum.motors[Left][Bottom].stop();
        mecanum.motors[Right][Top].stop();
        mecanum.motors[Right][Bottom].forward(speed);    
    }
    void backward (int speed)
    {
        mecanum.motors[Left][Top].stop();
        mecanum.motors[Left][Bottom].forward(speed);
        mecanum.motors[Right][Top].forward(speed);
        mecanum.motors[Right][Bottom].stop();
    }
} diagonal;
struct Mecaside
{
    public:
        Mecaside(int iSide)
        {
            side = iSide;
        }
        int side = 0;
        void forward(int speed)
        {
            mecanum.motors[side][0].forward(speed);
            mecanum.motors[side][1].forward(speed);
        }
        void backward(int speed)
        {
            mecanum.motors[side][0].backward(speed);
            mecanum.motors[side][1].backward(speed);
        }
        void stop()
        {
            mecanum.motors[side][0].stop();
            mecanum.motors[side][1].stop();
        }
};

/* Warning! All the pins are fakes and are here only for compilation test. For real upload, please change them with the real ones. */

Bluetooth bluetooth(&Serial1);
Mecaside left(Left);
Mecaside right(Right);
Report report(&Serial, true, 5 * (1000 / loopTime));
BlackLineSensor blackLine(A0, A1, A2);
HCSR04 backDistance(2, 3);

void setup ()
{
  // Serial setup //
  {
    Serial1.begin(9600);
    Serial.begin(9600);
    Serial.println("Computer serial communication's on...");
  }
  // Mecanum setup //
  {
    mecanum.stop();
  }
}

void loop ()
{
  if (bluetooth.receive())
  {
    if (bluetooth.lastError == DeserializationError::Ok)
    {
      report.ok++;
      serializeJson(bluetooth.json, Serial);
      bool motors = true;
      // AutoPilot //
      {
        void findTheLineToTheLeft ()
        {
          motors = false;
          mecanum.sideway.left(512);
          while (blackLine.getPattern != Position.Pattern.OnTheLine);
          mecanum.stop();
        }
        void findTheLineToTheRight ()
        {
          motors = false;
          mecanum.sideway.right(512);
          while (blackLine.getPattern != Position.Pattern.OnTheLine);
          mecanum.stop();
        }
        void goBackToCherry ()
        {
          motors = false;
          mecanum.backward(512);
          while (blackLine.getPattern() == Position.Pattern.OnTheLine && backDistance.getValue() != -1);
          if (blackLine.getPattern() != Position.Pattern.OnTheLine)
          {
            if (blackLine.getPattern() == Position.Pattern.OnTheRight)
            {
              mecanum.left.backward(512);
              mecanum.right.forward(512);
              while (blackLine.getPattern() != Position.Pattern.OnTheLine);
              mecanum.stop();
              goBackToCherry();
            }
          }
          else
          {
            mecanum.stop();
          }
        }
      }
      // Keybull //
      {
        switch (bluetooth.json["keybull"])
        {
          case 1:
            findTheLineToTheRight();
            goBackToCherry();
            break;
          case 2:
            break;
          case 3:
            findTheLineToTheLeft();
            goBackToCherry();
            break;

        }
      }
      // Motors //
      {
        if (!motors) return;
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
              mecanum.stop();
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
              mecanum.stop();
              break;
            default:
              if (value > 512) 
                diagonal.forward(value);
              else if (value < 512)
                diagonal.backward(value);
          }
        }
      }
      // Response //
      {
        bluetooth.json.clear();
        bluetooth.json["blackLine"]["pattern"] = blackLine.getPattern();
        bluetooth.json["blackLine"]["onTheLine"] = blackLine.lastPattern() == Position.Pattern.OnTheLine;
        bluetooth.send();
      }
    }
    else
    {
      report.inv++;
      bluetooth.empty();
    }
  }
  else
  {
    report.ntr++;
  }
  report.print();
  delay(loopTime);
}
