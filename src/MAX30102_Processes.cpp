#include "MAX30102_Processes.h"
#include <math.h>
#include <string.h>

namespace {
  // Parâmetros e estados
  int      g_sps        = 100;
  uint32_t g_ir_dc_min  = 0;

  // DC (filtro de 1ª ordem) — float é mais rápido no ESP32
  float    g_ir_dc      = 0.0f;
  float    g_red_dc     = 0.0f;
  constexpr float kAlphaDC = 0.01f; // porquê: rastrear DC lentamente

  // RMS (AC) acumulado por 1s
  float    g_ir_ac2_acc  = 0.0f;
  float    g_red_ac2_acc = 0.0f;
  unsigned g_acc_count   = 0;

  // Janela para média de R (segundos)
  constexpr int kWinSec = 12;
  float     g_Rbuf[kWinSec];
  unsigned  g_Rcount = 0;
  unsigned  g_Ridx   = 0;

  // Saídas
  float g_spo2     = 0.0f;
  bool  g_has_spo2 = false;

  float         g_bpm              = 0.0f;
  bool          g_has_bpm          = false;
  unsigned long g_sample_idx       = 0;
  unsigned long g_last_peak_sample = 0;
  float         g_prev2_ac         = 0.0f;
  float         g_prev1_ac         = 0.0f;

  // Refratário em amostras (≈ 300 ms)
  unsigned long g_refrac_samples   = 30;

  // Utilitários
  static inline float clampf(float x, float lo, float hi) {
    return (x < lo) ? lo : (x > hi) ? hi : x;
  }

  static inline bool finger_ok() {
    return (g_ir_dc_min == 0) || (g_ir_dc >= (float)g_ir_dc_min);
  }
}

void ppg_init(int sample_rate_sps)
{
  g_sps = (sample_rate_sps > 0 ? sample_rate_sps : 100);
  g_refrac_samples = (unsigned long)(0.30f * (float)g_sps); // porquê: evita contagem dupla de batimentos
  ppg_reset();
}

void ppg_reset()
{
  g_ir_dc = g_red_dc = 0.0f;
  g_ir_ac2_acc = g_red_ac2_acc = 0.0f;
  g_acc_count = 0;

  memset(g_Rbuf, 0, sizeof(g_Rbuf));
  g_Rcount = 0;
  g_Ridx   = 0;

  g_spo2     = 0.0f;
  g_has_spo2 = false;

  g_bpm              = 0.0f;
  g_has_bpm          = false;
  g_sample_idx       = 0;
  g_last_peak_sample = 0;
  g_prev2_ac = 0.0f;
  g_prev1_ac = 0.0f;
}

void ppg_setFingerThreshold(uint32_t ir_dc_min)
{
  g_ir_dc_min = ir_dc_min;
}

void ppg_feedSample(uint32_t red, uint32_t ir)
{
  // Atualiza DC (EWMA)
  g_ir_dc  = (1.0f - kAlphaDC)*g_ir_dc  + kAlphaDC*(float)ir;
  g_red_dc = (1.0f - kAlphaDC)*g_red_dc + kAlphaDC*(float)red;

  // AC instantâneo
  const float ir_ac  = (float)ir  - g_ir_dc;
  const float red_ac = (float)red - g_red_dc;

  // RMS por 1s
  g_ir_ac2_acc  += ir_ac  * ir_ac;
  g_red_ac2_acc += red_ac * red_ac;
  g_acc_count++;

  // Detecção de pico (IR), com dedo presente e limiar relativo
  const float        thr = 0.005f * g_ir_dc; // ≈0,5% do DC
  if (g_sample_idx >= 2 && finger_ok())
  {
    const bool is_peak = (g_prev1_ac > g_prev2_ac) &&
                         (g_prev1_ac > ir_ac)      &&
                         (g_prev1_ac > thr);

    const unsigned long since = g_sample_idx - g_last_peak_sample;

    if (is_peak && since > g_refrac_samples)
    {
      const unsigned long rr_samples =
        (g_last_peak_sample == 0) ? 0 : (g_sample_idx - g_last_peak_sample);

      g_last_peak_sample = g_sample_idx;

      if (rr_samples > 0)
      {
        const float bpm_inst = (60.0f * (float)g_sps) / (float)rr_samples;

        if (bpm_inst >= 30.0f && bpm_inst <= 220.0f)
        {
          // Suavização leve para estabilidade
          g_bpm     = g_has_bpm ? (0.8f * g_bpm + 0.2f * bpm_inst) : bpm_inst;
          g_has_bpm = true;
        }
      }
    }
  }

  // Avança histórico para próxima amostra
  g_prev2_ac = g_prev1_ac;
  g_prev1_ac = ir_ac;
  g_sample_idx++;
}

void ppg_tick_1s()
{
  // Se ficou tempo demais sem pico, invalida BPM (ex.: dedo saiu)
  if (g_last_peak_sample == 0 ||
      (g_sample_idx - g_last_peak_sample) > (unsigned long)(2.5f * (float)g_sps)) {
    g_has_bpm = false; // porquê: evita mostrar BPM "congelado"
  }

  if (g_acc_count == 0)
  {
    g_has_spo2 = false;
    return;
  }

  // RMS do último 1s
  const float ir_rms  = sqrtf(g_ir_ac2_acc  / (float)g_acc_count);
  const float red_rms = sqrtf(g_red_ac2_acc / (float)g_acc_count);

  // Zera acumuladores para a próxima janela de 1s
  g_ir_ac2_acc = g_red_ac2_acc = 0.0f;
  g_acc_count  = 0;

  // Pré-condições para R/SpO2
  const bool have_base =
    (g_ir_dc > 1.0f) && (g_red_dc > 1.0f) && (ir_rms > 0.0f) && (red_rms > 0.0f);

  if (!have_base || !finger_ok())
  {
    g_has_spo2 = false;
    return;
  }

  const float ratio_ir  = ir_rms  / g_ir_dc;
  const float ratio_red = red_rms / g_red_dc;
  if (ratio_ir <= 1e-9f)
  {
    g_has_spo2 = false;
    return;
  }

  const float R = (ratio_red / ratio_ir);

  // Atualiza média deslizante de R (janela de kWinSec s)
  g_Rbuf[g_Ridx] = R;
  if (g_Rcount < (unsigned)kWinSec) g_Rcount++;
  g_Ridx = (g_Ridx + 1) % kWinSec;

  if (g_Rcount < 6) // aguarda ~6s para estabilizar
  {
    g_has_spo2 = false;
    return;
  }

  float sumR = 0.0f;
  for (unsigned i = 0; i < g_Rcount; ++i) sumR += g_Rbuf[i];

  const float Ravg = (sumR / (float)g_Rcount);
  const float spo2 = 110.0f - 25.0f * Ravg; // aproximação linear típica

  g_spo2     = clampf(spo2, 70.0f, 100.0f);
  g_has_spo2 = true;
}

bool ppg_hasSpO2()  { return g_has_spo2; }
float ppg_getSpO2() { return g_spo2;     }

bool ppg_hasBPM()   { return g_has_bpm;  }
float ppg_getBPM()  { return g_bpm;      }
