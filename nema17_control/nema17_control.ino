// ===============================================
// Spresense MainCore
// A4988 + NEMA17 (1/16 microstep)
<<<<<<< HEAD
// attachTimerInterrupt() によるステッピング制御
// エンコーダはA相割り込み+B相判定（2逓倍）
// ホーム探索はループ式
// ===============================================

#include <Arduino.h>
#include "lgfx_user.hpp"
=======
// attachTimerInterrupt() を用いた通常運転制御
// 原点検出はループ制御で高精度停止
// LovyanGFX によるLCD表示付き
// ===============================================

#include <Arduino.h>
#include "lgfx_user.hpp"   // LovyanGFX 設定ヘッダ
>>>>>>> 8af195c9ff03a9c84f01334d3895f1d4701ba6c4

LGFX lcd;

// ===== パラメータ設定 =====
const int STEPS_PER_REV = 200 * 16;   // 1.8° × 1/16 = 3200 step/rev
const float MAX_RPM = 180.0f;          // 最大回転数 [rpm]
const float MIN_RPM = -180.0f;         // 最小回転数 [rpm]
const int HOME_OFFSET_STEPS = 0;     // 原点補正ステップ数
const float HOME_SPEED_RPM = 10.0f;    // ホーム探索速度 [rpm]

// ===== ピン設定 =====
const int DirPin   = 0;
const int StepPin  = 1;
const int EncA     = 2;
const int EncB     = 3;
const int HomePin  = 4;

<<<<<<< HEAD
// ===== モータパラメータ =====
const int STEPS_PER_REV = 200 * 16;   // 1.8° × 1/16 = 3200 step/rev
const float HOME_SPEED_RPM = 1.0f;    // ホーム探索速度
const int HOME_OFFSET_STEPS = 80;     // 原点補正
const float MAX_RPM = 240.0f;
const float MIN_RPM = -240.0f;

=======
>>>>>>> 8af195c9ff03a9c84f01334d3895f1d4701ba6c4
// ===== 状態変数 =====
volatile long encoderValue = 0;
volatile bool step_state = false;
volatile unsigned long current_interval = 0;  // μs単位
<<<<<<< HEAD
=======
volatile int rotation_dir = 1;
>>>>>>> 8af195c9ff03a9c84f01334d3895f1d4701ba6c4
volatile bool homed = false;
volatile int rotation_dir = 1;
float target_rpm = 0.0f;

<<<<<<< HEAD
// ===== エンコーダ割り込み（A相のみ） =====
void onEncoderA() {
  bool a = digitalRead(EncA);
  bool b = digitalRead(EncB);
  if (a == b)
    encoderValue++;
  else
    encoderValue--;
}

// ===== ステッピング割り込み =====
=======
// ===== エンコーダ状態保持 =====
volatile uint8_t lastAB = 0;
const int8_t encTable[16] = {
  0, -1, +1, 0,
  +1, 0, 0, -1,
  -1, 0, 0, +1,
  0, +1, -1, 0
};

// ===== 割り込みハンドラ（通常運転用） =====
>>>>>>> 8af195c9ff03a9c84f01334d3895f1d4701ba6c4
unsigned int onStepperTimer() {
  if (current_interval == 0) return 0;  // 停止
  digitalWrite(StepPin, step_state ? HIGH : LOW);
  step_state = !step_state;
  return (unsigned int)current_interval;
}

// ===== rpm → 周期変換 =====
unsigned long rpmToInterval(float rpm) {
  if (rpm == 0.0f) return 0;
  float stepsPerSec = (fabs(rpm) * STEPS_PER_REV) / 60.0f;
  float period_us = (1e6f / stepsPerSec) / 2.0f;  // 半周期
  return (unsigned long)period_us;
}

<<<<<<< HEAD
// ===== ホーム探索（ループ方式） =====
=======
// ===== エンコーダ読み込み（A/B両相監視） =====
void readEncoderAB() {
  uint8_t a = digitalRead(EncA);
  uint8_t b = digitalRead(EncB);
  uint8_t ab = (a << 1) | b;
  uint8_t idx = ((lastAB << 2) | ab) & 0x0F;
  encoderValue += encTable[idx];
  lastAB = ab;
}

// ===== ホームポジション探索（ループ方式） =====
>>>>>>> 8af195c9ff03a9c84f01334d3895f1d4701ba6c4
void homeRoutine_loop() {
  Serial.println("Homing start...");
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_YELLOW);
  lcd.setTextSize(2);
  lcd.setCursor(20, 140);
  lcd.println("Homing...");

<<<<<<< HEAD
  digitalWrite(DirPin, LOW);   // 原点方向へ
  rotation_dir = 1;

  const float rpm = HOME_SPEED_RPM;
  const float steps_per_sec = (rpm * STEPS_PER_REV) / 60.0f;
  const unsigned long delay_us = (1e6f / steps_per_sec) / 2.0f;

  int stepCount = 0;
  while (true) {
=======
  // ==== 回転方向設定 ====
  digitalWrite(DirPin, LOW);   // 原点方向へ回転
  rotation_dir = 1;

  const float rpm = HOME_SPEED_RPM;  // ホーム探索速度
  const float steps_per_sec = (rpm * STEPS_PER_REV) / 60.0f;
  const unsigned long delay_us = (1e6f / steps_per_sec) / 2.0f;

  // ==== 探索ループ ====
  int stepCount = 0;
  while (true) {
    // 1パルス出力
>>>>>>> 8af195c9ff03a9c84f01334d3895f1d4701ba6c4
    digitalWrite(StepPin, HIGH);
    delayMicroseconds(delay_us);
    digitalWrite(StepPin, LOW);
    delayMicroseconds(delay_us);
    stepCount++;

<<<<<<< HEAD
    // 原点センサチェック
    if (digitalRead(HomePin) == LOW) {
      delay(5);
=======
    // センサ読み取り（LOWが検出）
    if (digitalRead(HomePin) == LOW) {
      delay(5); // チャタリング除去
>>>>>>> 8af195c9ff03a9c84f01334d3895f1d4701ba6c4
      if (digitalRead(HomePin) == LOW) {
        Serial.println("Home detected!");
        break;
      }
    }

<<<<<<< HEAD
    // セーフガード：3回転以上探したら中断
=======
    // 安全策：3回転分探して見つからなければ中断
>>>>>>> 8af195c9ff03a9c84f01334d3895f1d4701ba6c4
    if (stepCount > STEPS_PER_REV * 3) {
      Serial.println("Home not detected (timeout)");
      break;
    }
  }

<<<<<<< HEAD
  // --- オフセット補正 ---
=======
  // ==== オフセット補正 ====
>>>>>>> 8af195c9ff03a9c84f01334d3895f1d4701ba6c4
  Serial.println("Offset correction...");
  digitalWrite(DirPin, (HOME_OFFSET_STEPS > 0) ? LOW : HIGH);
  for (int i = 0; i < abs(HOME_OFFSET_STEPS); i++) {
    digitalWrite(StepPin, HIGH);
    delayMicroseconds(delay_us);
    digitalWrite(StepPin, LOW);
    delayMicroseconds(delay_us);
  }

<<<<<<< HEAD
  // --- 初期化 ---
=======
  // ==== 完了処理 ====
>>>>>>> 8af195c9ff03a9c84f01334d3895f1d4701ba6c4
  current_interval = 0;
  target_rpm = 0.0f;
  rotation_dir = 1;
  homed = true;

  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_GREEN);
  lcd.setCursor(40, 140);
  lcd.println("Home complete!");
  Serial.println("Home + offset complete.");
  delay(1000);
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
<<<<<<< HEAD
  pinMode(HomePin, INPUT);

  attachInterrupt(digitalPinToInterrupt(EncA), onEncoderA, CHANGE);
  attachTimerInterrupt(onStepperTimer, (unsigned int)current_interval);

  digitalWrite(DirPin, LOW);
  homeRoutine_loop();  // 原点動作
=======
  pinMode(HomePin, INPUT);  // 外部センサが駆動するためプルアップなし

  // エンコーダ初期状態
  lastAB = (digitalRead(EncA) << 1) | digitalRead(EncB);
  attachInterrupt(digitalPinToInterrupt(EncA), readEncoderAB, CHANGE);
  attachInterrupt(digitalPinToInterrupt(EncB), readEncoderAB, CHANGE);

  // タイマ割り込み開始（まだ0で停止状態）
  attachTimerInterrupt(onStepperTimer, (unsigned int)current_interval);

  digitalWrite(DirPin, LOW);

  // 原点動作（ループ方式）
  homeRoutine_loop();
>>>>>>> 8af195c9ff03a9c84f01334d3895f1d4701ba6c4
}

// ===== メインループ =====
void loop() {
  if (!homed) return;

  // --- エンコーダ入力による速度調整 ---
<<<<<<< HEAD
  static long lastEncoder = 0;
  long enc = encoderValue - lastEncoder;
  lastEncoder = encoderValue;

  if (enc != 0) {
    target_rpm += enc * 2;  // 1カウントで2rpm変化
    if (target_rpm > MAX_RPM) target_rpm = MAX_RPM;
    if (target_rpm < MIN_RPM) target_rpm = MIN_RPM;
  }
=======
  int enc = encoderValue;
  encoderValue = 0;
  target_rpm += enc * 2;   // エンコーダ1カウントあたり2rpm変化

  // --- 上下限制御 ---
  if (target_rpm > MAX_RPM) target_rpm = MAX_RPM;
  if (target_rpm < MIN_RPM) target_rpm = MIN_RPM;
>>>>>>> 8af195c9ff03a9c84f01334d3895f1d4701ba6c4

  // --- 回転方向設定 ---
  if (target_rpm > 0) rotation_dir = 1;
  else if (target_rpm < 0) rotation_dir = -1;
  digitalWrite(DirPin, (rotation_dir == 1) ? LOW : HIGH);

  // --- タイマ周期更新 ---
  unsigned long new_int = rpmToInterval(target_rpm);
  if (new_int != current_interval) {
    current_interval = new_int;
    attachTimerInterrupt(onStepperTimer, (unsigned int)current_interval);
  }

  // --- LCD表示更新 ---
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

  delay(30);
}
