/*
  Copyright (c) 2019, miya
  Modified by airpocket, 2025
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
  Spresense Dynamic Sine Wave Generator (Non-blocking Serial Input)
  - MediaPlayer + OutputMixer 構成はオリジナルに準拠
  - 周波数をシリアル入力（非ブロッキング）で動的に変更可能
  - 入力例: 440 [Enter] で 440Hz に切り替え
*/

#include <MediaPlayer.h>
#include <OutputMixer.h>
#include <MemoryUtil.h>
#include <math.h>

const int MIXER_VOLUME = -160;
const int32_t S_BUFFER_SIZE = 8192;
uint8_t s_buffer[S_BUFFER_SIZE];
bool err_flag = false;

// ======= 波形パラメータ =======
const float SAMPLE_RATE = 48000.0f;
float freq = 440.0f;          // 初期周波数(Hz)
const int AMPLITUDE = 15000;  // 音量（最大32767）
float phase = 0.0f;
const float PI2 = 6.2831853f;

// ======= Audio関連 =======
MediaPlayer *player;
OutputMixer *mixer;

// ======= コールバック =======
static void error_callback(const ErrorAttentionParam *errparam)
{
  if (errparam->error_code > AS_ATTENTION_CODE_WARNING) {
    err_flag = true;
  }
}

static void mixer_done_callback(MsgQueId id, MsgType type, AsOutputMixDoneParam *param)
{
  return;
}

static void mixer_send_callback(int32_t id, bool is_end)
{
  AsRequestNextParam next;
  next.type = (!is_end) ? AsNextNormalRequest : AsNextStopResRequest;
  AS_RequestNextPlayerProcess(AS_PLAYER_ID_0, &next);
  return;
}

static bool player_done_callback(AsPlayerEvent event, uint32_t result, uint32_t sub_result)
{
  return true;
}

// ======= 波形生成関数 =======
void generate_sine(int16_t *buf, uint32_t frames)
{
  float phase_inc = PI2 * freq / SAMPLE_RATE;
  for (uint32_t i = 0; i < frames; i++) {
    int16_t v = (int16_t)(sin(phase) * AMPLITUDE);
    buf[i * 2 + 0] = v; // Left
    buf[i * 2 + 1] = v; // Right
    phase += phase_inc;
    if (phase >= PI2) phase -= PI2;
  }
}

// ======= PCMデコードコールバック =======
void player_decode_callback(AsPcmDataParam pcm_param)
{
  int16_t *buf = (int16_t*)pcm_param.mh.getPa();
  uint32_t frames = pcm_param.size / 4; // (16bit * 2ch) = 4 bytes/frame

  generate_sine(buf, frames);
  mixer->sendData(OutputMixer0, mixer_send_callback, pcm_param);
}

// ======= 初期化 =======
void setup()
{
  Serial.begin(115200);
  Serial.println("=== Spresense: Dynamic Sine Wave Generator ===");
  Serial.println("Enter frequency in Hz (e.g. 440) and press Enter.");

  initMemoryPools();
  createStaticPools(MEM_LAYOUT_PLAYER);

  player = MediaPlayer::getInstance();
  mixer = OutputMixer::getInstance();

  player->begin();
  mixer->activateBaseband();

  player->create(MediaPlayer::Player0, error_callback);
  mixer->create(error_callback);
  player->activate(MediaPlayer::Player0, player_done_callback);
  mixer->activate(OutputMixer0, mixer_done_callback);

  usleep(100 * 1000);
  player->init(MediaPlayer::Player0, AS_CODECTYPE_WAV, "/mnt/sd0/BIN",
               AS_SAMPLINGRATE_48000, AS_BITLENGTH_16, AS_CHANNEL_STEREO);
  mixer->setVolume(MIXER_VOLUME, 0, 0);

  memset(s_buffer, 0, sizeof(s_buffer));
  player->writeFrames(MediaPlayer::Player0, s_buffer, S_BUFFER_SIZE);
  player->start(MediaPlayer::Player0, player_decode_callback);

  Serial.print("[OK] Audio initialized. Current freq = ");
  Serial.print(freq);
  Serial.println(" Hz");
  Serial.println("----------------------------------------------");
}

// ======= メインループ =======
void loop()
{
  player->writeFrames(MediaPlayer::Player0, s_buffer, S_BUFFER_SIZE);

  // === ノンブロッキング周波数入力 ===
  static String inputStr = "";
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputStr.length() > 0) {
        float newFreq = inputStr.toFloat();
        inputStr = "";
        if (newFreq > 20.0 && newFreq < 20000.0) {
          freq = newFreq;
          Serial.print("[INFO] Frequency changed to ");
          Serial.print(freq);
          Serial.println(" Hz");
        } else {
          Serial.println("[WARN] Invalid frequency. (20–20000Hz only)");
        }
      }
    } else if (isDigit(c) || c == '.') {
      inputStr += c;
    } else {
      // 無効文字は無視
    }
  }

  // === エラーチェック ===
  if (err_flag) {
    Serial.println("[ERROR] Audio system halted!");
    player->stop(MediaPlayer::Player0);
    while (1);
  }

  // === デバッグ出力 ===
  static unsigned long last_debug = 0;
  if (millis() - last_debug > 1000) {
    last_debug = millis();
    Serial.print("[INFO] phase="); Serial.print(phase, 4);
    Serial.print("  freq="); Serial.print(freq);
    Serial.println(" Hz");
  }

  usleep(100);
}
