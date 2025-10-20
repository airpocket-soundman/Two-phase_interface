// ===============================================
// Spresense MainCore
// A4988 + NEMA17 (1/16 microstep)
// attachTimerInterrupt() を用いたタイマ割り込み方式
// 原点検出 + 原点補正 + -60〜+60 rpm制御 + 速度表示(LovyanGFX)
// フルクアドラチャ方式エンコーダ読み込み
// ===============================================

#include <Arduino.h>
#include "lgfx_user.hpp"   // あなたの LovyanGFX 設定ヘッダ

LGFX lcd;  // 画面オブジェクト

// ===== ピン設定 =====
const int DirPin   = 0;
const int StepPin  = 1;
const int EncA     = 2;
const int EncB     = 3;
const int HomePin  = 4;

// ===== モータパラメータ =====
const int STEPS_PER_REV = 200 * 16;  // 1.8° × 1/16 = 3200 step/rev

// ===== 状態変数 =====
volatile int encoderValue = 0;
volatile float target_rpm = 0.0f;
volatile bool step_state = false;
volatile unsigned long current_interval = 100;  // μs単位
volatile int rotation_dir = 1;
volatile bool homed = false;

// ===== 原点補正 =====
const int home_offset_steps = 80;

// ===== エンコーダ状態保持 =====
volatile uint8_t lastAB = 0;
const int8_t encTable[16] = {
  0, -1, +1, 0,
  +1, 0, 0, -1,
  -1, 0, 0, +1,
  0, +1, -1, 0
};

// ===== 割り込みハンドラ（タイマ） =====
unsigned int onStepperTimer() {
  if (current_interval == 0) return 0;  // 停止
  digitalWrite(StepPin, step_state ? HIGH : LOW);
  step_state = !step_state;
  return (unsigned int)current_interval;  // 次回も同周期
}

// ===== rpm→間隔変換 =====
unsigned long rpmToInterval(float rpm) {
  if (rpm == 0.0f) return 0;
  float stepsPerSec = (fabs(rpm) * STEPS_PER_REV) / 60.0f;
  float period_us = (1e6f / stepsPerSec) / 2.0f;  // 半周期
  return (unsigned long)period_us;
}

// ===== エンコーダ読み込み（A/B両相監視） =====
void readEncoderAB() {
  uint8_t a = digitalRead(EncA);
  uint8_t b = digitalRead(EncB);
  uint8_t ab = (a << 1) | b;
  uint8_t idx = ((lastAB << 2) | ab) & 0x0F;  // 前回状態→今回状態
  encoderValue += encTable[idx];
  lastAB = ab;
}

// ===== 原点検出ルーチン =====
void homeRoutine() {
  Serial.println("Homing start...");
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_YELLOW);
  lcd.setTextSize(2);
  lcd.setCursor(20, 140);
  lcd.println("Homing...");

  digitalWrite(DirPin, LOW);
  rotation_dir = 1;
  float rpm = 1.0f;
  float steps_per_sec = (rpm * STEPS_PER_REV) / 60.0f;
  unsigned long delay_us = (1e6f / steps_per_sec) / 2.0f;

  // --- 原点センサ立下がりまで回転 ---
  while (digitalRead(HomePin) == HIGH) {
    digitalWrite(StepPin, HIGH);
    delayMicroseconds(delay_us);
    digitalWrite(StepPin, LOW);
    delayMicroseconds(delay_us);
  }

  Serial.println("Home detected. Doing offset correction...");

  // --- 原点補正 ---
  for (int i = 0; i < abs(home_offset_steps); i++) {
    digitalWrite(DirPin, (home_offset_steps > 0) ? LOW : HIGH);
    digitalWrite(StepPin, HIGH);
    delayMicroseconds(delay_us);
    digitalWrite(StepPin, LOW);
    delayMicroseconds(delay_us);
  }

  // --- 停止状態に初期化 ---
  current_interval = 0;
  target_rpm = 0.0f;
  rotation_dir = 1;
  homed = true;

  Serial.println("Home + offset complete.");
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_GREEN);
  lcd.setCursor(40, 140);
  lcd.println("Home complete!");
  delay(1000);

  // ★ 画面クリアを追加（重なり防止）
  lcd.fillScreen(TFT_BLACK);
}

// ===== 初期化 =====
void setup() {
  Serial.begin(115200);
  Serial.println("Stepper control start");

  lcd.init();
  lcd.setRotation(1);
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(2);
  lcd.setCursor(10, 10);
  lcd.println("Stepper RPM Display");

  pinMode(DirPin, OUTPUT);
  pinMode(StepPin, OUTPUT);
  pinMode(EncA, INPUT_PULLUP);
  pinMode(EncB, INPUT_PULLUP);
  pinMode(HomePin, INPUT_PULLUP);

  // エンコーダ初期状態取得
  lastAB = (digitalRead(EncA) << 1) | digitalRead(EncB);

  // 両相割り込み
  attachInterrupt(digitalPinToInterrupt(EncA), readEncoderAB, CHANGE);
  attachInterrupt(digitalPinToInterrupt(EncB), readEncoderAB, CHANGE);

  // タイマ割り込み開始
  attachTimerInterrupt(onStepperTimer, (unsigned int)current_interval);

  digitalWrite(DirPin, LOW);

  // 原点動作
  homeRoutine();
}

// ===== メインループ =====
void loop() {
  if (!homed) return;

  // --- エンコーダ入力による速度調整 ---
  int enc = encoderValue;
  encoderValue = 0;
  target_rpm += enc * 2;   // エンコーダ1カウントあたり2rpm変化
  if (target_rpm > 60) target_rpm = 60;
  if (target_rpm < -60) target_rpm = -60;

  // --- 回転方向設定 ---
  if (target_rpm > 0) rotation_dir = 1;
  else if (target_rpm < 0) rotation_dir = -1;
  digitalWrite(DirPin, (rotation_dir == 1) ? LOW : HIGH);

  // --- タイマ周期変更 ---
  unsigned long new_int = rpmToInterval(target_rpm);
  if (new_int != current_interval) {
    current_interval = new_int;
    attachTimerInterrupt(onStepperTimer, (unsigned int)current_interval);
  }

  // --- LCD更新 ---
  static unsigned long lastDisplay = 0;
  if (millis() - lastDisplay > 200) {
    lcd.fillRect(0, 60, 240, 80, TFT_BLACK);
    lcd.setTextSize(4);
    lcd.setTextColor(TFT_CYAN);
    lcd.setCursor(20, 80);
    lcd.printf("RPM: %4.1f", target_rpm);

    lcd.setTextSize(2);
    lcd.setTextColor(TFT_YELLOW);
    lcd.setCursor(20, 140);
    if (target_rpm == 0)
      lcd.print("STOP");
    else if (rotation_dir == 1)
      lcd.print("DIR: CW  ");
    else
      lcd.print("DIR: CCW");

    lastDisplay = millis();
  }

  delay(50);
}

