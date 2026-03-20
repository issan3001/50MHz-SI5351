#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <si5351.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define BUTTON_PIN 12
#define BTN_UP 14
#define BTN_DOWN 26

#define LED_R 15
#define LED_G 2
#define LED_B 4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Si5351 si5351;

bool tx = false;
bool lastButton = HIGH;
bool lastUp = HIGH;
bool lastDown = HIGH;

bool lastSelectUp = HIGH;
bool lastSelectDown = HIGH;

uint64_t freq = 50000000ULL;

bool selecting = false;
bool dangerMode = false;

unsigned long lastBlink = 0;
unsigned long bothPressStart = 0;

bool blinkState = false;
bool bothPressed = false;

// =====================
String formatFreq(uint64_t f)
{
  char buf[20];
  sprintf(buf, "%02llu.%03llu",
          f / 1000000,
          (f % 1000000) / 1000);
  return String(buf);
}

// =====================
void setFrequency()
{
  si5351.set_freq(freq * 100ULL, SI5351_CLK0);
}

// =====================
void drawDanger()
{
  display.clearDisplay();

  if(blinkState){
    display.drawRect(0,0,128,64,WHITE);
  }

  display.setCursor(15,10);
  display.print("!!! CAUTION !!!");

  display.setCursor(5,25);
  display.print("in this mode you can");

  display.setCursor(5,35);
  display.print("not connect antenna");

  if(blinkState){
    display.fillTriangle(5,60,10,50,15,60,WHITE);
  }

  display.setCursor(15,50);
  display.print("YES(UP)");

  display.setCursor(80,50);
  display.print("NO(DOWN)");

  display.display();
}

// =====================
void drawUI()
{
  if(selecting){
    drawDanger();
    return;
  }

  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setCursor(0,0);
  display.print("TX: ");

  if(tx){
    display.fillRect(28,0,15,8,WHITE);
    display.setTextColor(BLACK);
    display.setCursor(30,0);
    display.print("ON");
    display.setTextColor(WHITE);
  } else {
    display.setCursor(30,0);
    display.print("OFF");
  }

  display.drawLine(0,10,128,10,WHITE);

  display.setTextSize(2);
  String f = formatFreq(freq) + " MHz";

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(f, 0, 0, &x1, &y1, &w, &h);
  int x = (128 - w) / 2;

  display.setCursor(x,18);
  display.print(f);

  display.drawLine(0,40,128,40,WHITE);

  display.setTextSize(1);
  display.setCursor(0,44);

  if(dangerMode){
    display.print("!!! DANGER MODE !!!");
  } else {
    display.println("TEST VFO");
    display.println("GENERATOR");
  }

  display.display();
}

// =====================
void setup()
{
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);

  Wire.begin(21,22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 25000000, 0);
  setFrequency();
  si5351.output_enable(SI5351_CLK0, 0);

  drawUI();
}

// =====================
void loop()
{
  // 点滅
  if(millis() - lastBlink > 300){
    blinkState = !blinkState;
    lastBlink = millis();
    drawUI();
  }

  // LED制御
  if(dangerMode){
    digitalWrite(LED_R, blinkState);
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_B, LOW);
  } else {
    if(tx){
      digitalWrite(LED_R, HIGH);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
    } else {
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, HIGH);
    }
  }

  // 同時押し
  bool upNow = digitalRead(BTN_UP) == LOW;
  bool downNow = digitalRead(BTN_DOWN) == LOW;

  if(upNow && downNow){
    if(!bothPressed){
      bothPressStart = millis();
      bothPressed = true;
    }

    if(millis() - bothPressStart > 10000){
      selecting = true;
      drawUI();
    }
  } else {
    bothPressed = false;
  }

  // 選択
  if(selecting){
    bool upSel = digitalRead(BTN_UP);
    bool downSel = digitalRead(BTN_DOWN);

    if(lastSelectUp == HIGH && upSel == LOW){
      freq = 80000000ULL;
      setFrequency();
      selecting = false;
      dangerMode = true;
      delay(300);
    }

    if(lastSelectDown == HIGH && downSel == LOW){
      selecting = false;
      delay(300);
    }

    lastSelectUp = upSel;
    lastSelectDown = downSel;
    return;
  }

  // TX
  bool current = digitalRead(BUTTON_PIN);

  if(lastButton == HIGH && current == LOW)
  {
    tx = !tx;
    si5351.output_enable(SI5351_CLK0, tx);
    drawUI();
    delay(200);
  }
  lastButton = current;

  // +0.1
  bool up = digitalRead(BTN_UP);
  if(lastUp == HIGH && up == LOW)
  {
    if(!tx){
      if(freq < 52000000ULL){
        freq += 100000ULL;
        setFrequency();
      }
    }
    drawUI();
    delay(150);
  }
  lastUp = up;

  // -0.1
  bool down = digitalRead(BTN_DOWN);
  if(lastDown == HIGH && down == LOW)
  {
    if(!tx){
      if(freq > 50000000ULL){
        freq -= 100000ULL;
        setFrequency();
      }
    }
    drawUI();
    delay(150);
  }
  lastDown = down;
}
