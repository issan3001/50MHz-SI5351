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

// ★ 50MHzスタート
uint64_t freq = 50000000ULL;

// TX LOCK
bool showLock = false;
unsigned long lockTime = 0;

// 点滅
bool blinkState = false;
unsigned long lastBlink = 0;

// =====================
// 表示
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
// 周波数設定（CLK0固定）
// =====================
void setFrequency()
{
  si5351.set_freq(freq * 100ULL, SI5351_CLK0);
}

// =====================
// UI
// =====================
void drawUI()
{
  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("TX: ");

  if(tx){
    display.fillRect(28,0,15,8,WHITE);
    display.setTextColor(BLACK);
    display.setCursor(30,0);
    display.print("ON");
    display.setTextColor(WHITE);

    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_B, LOW);
  } else {
    display.setCursor(30,0);
    display.print("OFF");

    digitalWrite(LED_R, LOW);
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_B, HIGH);
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

  if(showLock && millis() - lockTime < 1500){

    if(blinkState){
      display.fillTriangle(0, 54, 6, 44, 12, 54, WHITE);
    }

    display.setCursor(16,44);
    display.print("TX LOCK");

  } else {
    display.println("TEST VFO");
    display.println("GENERATOR");
  }

  display.display();
}

// =====================
// SETUP
// =====================
void setup()
{
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);

  Serial.begin(115200);

  Wire.begin(21,22);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // ★ SI5351初期化（25MHz水晶）
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 25000000, 0);

  // ★ 50MHzをCLK0に設定
  setFrequency();

  // ★ CLK0のみ有効化
  si5351.output_enable(SI5351_CLK0, 0);

  drawUI();
}

// =====================
// LOOP
// =====================
void loop()
{
  // 点滅
  if(millis() - lastBlink > 300){
    blinkState = !blinkState;
    lastBlink = millis();
    if(showLock) drawUI();
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

  // +0.1MHz
  bool up = digitalRead(BTN_UP);
  if(lastUp == HIGH && up == LOW)
  {
    if(tx){
      showLock = true;
      lockTime = millis();
    } else {
      if(freq < 52000000ULL){
        freq += 100000ULL;
        setFrequency();
      } else {
        showLock = true;
        lockTime = millis();
      }
    }
    drawUI();
    delay(150);
  }
  lastUp = up;

  // -0.1MHz
  bool down = digitalRead(BTN_DOWN);
  if(lastDown == HIGH && down == LOW)
  {
    if(tx){
      showLock = true;
      lockTime = millis();
    } else {
      if(freq > 50000000ULL){
        freq -= 100000ULL;
        setFrequency();
      } else {
        showLock = true;
        lockTime = millis();
      }
    }
    drawUI();
    delay(150);
  }
  lastDown = down;

  // 表示戻し
  if(showLock && millis() - lockTime > 1500){
    showLock = false;
    drawUI();
  }
}
