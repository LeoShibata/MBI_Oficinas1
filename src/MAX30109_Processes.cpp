#include "MAX30109_Processes.h"
#include <math.h>
#include <string.h>

namespace {
  int      g_sps        = 100;
  uint32_t g_ir_dc_min  = 0;

  double   g_ir_dc      = 0.0;
  double   g_red_dc     = 0.0;
  const double kAlphaDC = 0.01;

  double   g_ir_ac2_acc  = 0.0;
  double   g_red_ac2_acc = 0.0;
  unsigned g_acc_count   = 0;

  constexpr int kWinSec = 12;
  float     g_Rbuf[kWinSec];
  unsigned  g_Rcount = 0;
  unsigned  g_Ridx   = 0;

  float g_spo2     = 0.0f;
  bool  g_has_spo2 = false;

  float         g_bpm              = 0.0f;
  bool          g_has_bpm          = false;
  unsigned long g_sample_idx       = 0;
  unsigned long g_last_peak_sample = 0;
  double        g_prev2_ac         = 0.0;
  double        g_prev1_ac         = 0.0;

  static inline float clampf(float x, float lo, float hi)
  {
    return (x < lo) ? lo : (x > hi) ? hi : x;
  }

  static inline bool finger_ok()
  {
    return (g_ir_dc_min == 0) || (g_ir_dc >= (double)g_ir_dc_min);
  }
}

void ppg_init(int sample_rate_sps)
{
  g_sps = (sample_rate_sps > 0 ? sample_rate_sps : 100);
  ppg_reset();
}

void ppg_reset()
{
  g_ir_dc = g_red_dc = 0.0;
  g_ir_ac2_acc = g_red_ac2_acc = 0.0;
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
  g_prev2_ac = 0.0;
  g_prev1_ac = 0.0;
}

void ppg_setFingerThreshold(uint32_t ir_dc_min)
{
  g_ir_dc_min = ir_dc_min;
}

void ppg_feedSample(uint32_t red, uint32_t ir)
{
  g_ir_dc  = (1.0 - kAlphaDC)*g_ir_dc  + kAlphaDC*(double)ir;
  g_red_dc = (1.0 - kAlphaDC)*g_red_dc + kAlphaDC*(double)red;

  const double ir_ac  = (double)ir  - g_ir_dc;
  const double red_ac = (double)red - g_red_dc;

  g_ir_ac2_acc  += ir_ac  * ir_ac;
  g_red_ac2_acc += red_ac * red_ac;
  g_acc_count++;

  const double        thr    = 0.005 * g_ir_dc;
  const unsigned long refrac = (unsigned long)(0.30 * g_sps);

  if (g_sample_idx >= 2)
  {
    const bool is_peak = (g_prev1_ac > g_prev2_ac) &&
                         (g_prev1_ac > ir_ac)      &&
                         (g_prev1_ac > thr);

    const unsigned long since = g_sample_idx - g_last_peak_sample;

    if (is_peak && since > refrac)
    {
      const unsigned long rr_samples =
        (g_last_peak_sample == 0) ? 0 : (g_sample_idx - g_last_peak_sample);

      g_last_peak_sample = g_sample_idx;

      if (rr_samples > 0)
      {
        const float bpm_inst = (60.0f * (float)g_sps) / (float)rr_samples;

        if (bpm_inst >= 30.0f && bpm_inst <= 220.0f)
        {
          if (g_has_bpm)
          {
            g_bpm = 0.8f * g_bpm + 0.2f * bpm_inst;
          }
          else
          {
            g_bpm    = bpm_inst;
            g_has_bpm = true;
          }
        }
      }
    }
  }

  g_prev2_ac = g_prev1_ac;
  g_prev1_ac = ir_ac;
  g_sample_idx++;
}

void ppg_tick_1s()
{
  if (g_acc_count == 0)
  {
    g_has_spo2 = false;
    return;
  }

  const double ir_rms  = sqrt(g_ir_ac2_acc  / (double)g_acc_count);
  const double red_rms = sqrt(g_red_ac2_acc / (double)g_acc_count);

  g_ir_ac2_acc = g_red_ac2_acc = 0.0;
  g_acc_count  = 0;

  const bool have_base =
    (g_ir_dc > 1.0) && (g_red_dc > 1.0) && (ir_rms > 0.0) && (red_rms > 0.0);

  if (!have_base || !finger_ok())
  {
    g_has_spo2 = false;
    return;
  }

  const double ratio_ir  = ir_rms  / g_ir_dc;
  const double ratio_red = red_rms / g_red_dc;
  if (ratio_ir <= 1e-9)
  {
    g_has_spo2 = false;
    return;
  }

  const float R = (float)(ratio_red / ratio_ir);

  g_Rbuf[g_Ridx] = R;
  if (g_Rcount < (unsigned)kWinSec)
  {
    g_Rcount++;
  }
  g_Ridx = (g_Ridx + 1) % kWinSec;

  if (g_Rcount < 6)
  {
    g_has_spo2 = false;
    return;
  }

  double sumR = 0.0;
  for (unsigned i = 0; i < g_Rcount; ++i)
  {
    sumR += g_Rbuf[i];
  }

  const float Ravg = (float)(sumR / (double)g_Rcount);
  float spo2 = 110.0f - 25.0f * Ravg;

  g_spo2     = clampf(spo2, 70.0f, 100.0f);
  g_has_spo2 = true;
}

bool ppg_hasSpO2()
{
  return g_has_spo2;
}

float ppg_getSpO2()
{
  return g_spo2;
}

bool ppg_hasBPM()
{
  return g_has_bpm;
}

float ppg_getBPM()
{
  return g_bpm;
}
