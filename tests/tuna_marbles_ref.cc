// SPDX-License-Identifier: MIT
// Bit-exact check of TUNA's C port of Marbles (firmware/src/tuna_marbles.c) against Mutable
// Instruments' original C++ (not part of this tree: run with a checkout of pichenettes/eurorack
// and pichenettes/stmlib). Build and run with tests/tuna_marbles_ref.sh, which also applies the
// port's one deliberate change (a clamped table index in the lag processor) to the original.
//
//   ./tuna_marbles_ref [SCENARIOS [ONLY]]   ONLY: report just that scenario (VERBOSE=1 dumps it)
//
// Each scenario seeds both implementations alike, then renders blocks while parameters change at
// random (models, rate, range, bias, jitter, deja vu, length, pulse width, clock source, spread,
// steps, ranges, control modes, scales, Y ratio, resets) and compares every ramp, gate and voltage.
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "marbles/random/random_generator.h"
#include "marbles/random/random_stream.h"
#include "marbles/random/t_generator.h"
#include "marbles/random/x_y_generator.h"

extern "C" {
#include "tuna_marbles.h"
#include "tuna_theory.h"
}

using namespace marbles;

static const size_t kMaxBlock = 64;
static const float kSampleRate = 32000.0f;

// The originals keep uninitialised members (RampDivider::reset_at_next_pulse_); static storage
// zero-fills them, as on the module.
static RandomGenerator ref_generator;
static RandomStream ref_stream;
static TGenerator ref_t;
static XYGenerator ref_xy;

static tm_rng port_rng;
static tm_tgen port_t;
static tm_xygen port_xy;

static uint32_t test_state = 1;
static uint32_t rnd() { test_state = test_state * 1103515245u + 12345u; return test_state >> 8; }
static float frnd() { return (rnd() & 0xFFFF) / 65535.0f; }
static int irnd(int n) { return (int)(rnd() % (uint32_t)n); }

struct Settings {
  int model, range, clock_source, scale_kind;
  float rate, bias, jitter, deja_vu, pw_mean, pw_std;
  int length;
  tm_group x, y;
};

static float pick_unit() {
  // exercise the corners as well as the middle
  switch (irnd(6)) {
    case 0: return 0.0f;
    case 1: return 1.0f;
    case 2: return 0.5f;
    default: return frnd();
  }
}

static void randomize(Settings* s) {
  s->model = irnd(TM_T_MODEL_COUNT);
  s->range = irnd(3);
  s->clock_source = irnd(4);
  s->scale_kind = irnd(TS_COUNT + 1);
  s->rate = -48.0f + 96.0f * frnd();
  s->bias = pick_unit();
  s->jitter = pick_unit();
  s->deja_vu = pick_unit();
  s->pw_mean = pick_unit();
  s->pw_std = irnd(3) ? 0.0f : frnd();
  s->length = 1 + irnd(16);
  tm_group* g[2] = {&s->x, &s->y};
  for (int i = 0; i < 2; ++i) {
    g[i]->control_mode = irnd(3);
    g[i]->voltage_range = irnd(3);
    g[i]->spread = pick_unit();
    g[i]->bias = pick_unit();
    g[i]->steps = pick_unit();
    g[i]->deja_vu = pick_unit();
    g[i]->length = 1 + irnd(16);
    static const tm_ratio ratios[] = {{1, 4}, {1, 2}, {1, 1}, {2, 1}, {3, 2}, {1, 8}};
    g[i]->ratio = ratios[irnd(6)];
  }
  // Marbles drives Y with CONTROL_MODE_IDENTICAL only: BUMP/TILT spread over X1..X3, and on the
  // fourth channel TILT would push bias and spread outside 0..1.
  s->y.control_mode = TM_CONTROL_IDENTICAL;
}

static void apply(const Settings& s) {
  // T
  ref_t.set_model(static_cast<TGeneratorModel>(s.model));
  ref_t.set_range(static_cast<TGeneratorRange>(s.range));
  ref_t.set_rate(s.rate);
  ref_t.set_bias(s.bias);
  ref_t.set_jitter(s.jitter);
  ref_t.set_deja_vu(s.deja_vu);
  ref_t.set_length(s.length);
  ref_t.set_pulse_width_mean(s.pw_mean);
  ref_t.set_pulse_width_std(s.pw_std);
  port_t.model = s.model;
  port_t.range = s.range;
  port_t.rate = s.rate;
  port_t.bias = s.bias;
  port_t.jitter = s.jitter;
  port_t.sequence.deja_vu = s.deja_vu;
  tm_seq_set_length(&port_t.sequence, s.length);
  port_t.pulse_width_mean = s.pw_mean;
  port_t.pulse_width_std = s.pw_std;
}

static void load_scale(int kind) {
  tm_scale ps;
  if (kind == TS_COUNT)
    tm_scale_init_major(&ps);
  else
    tm_scale_from_mask(&ps, tt_scale_mask(kind));
  Scale rs;
  rs.base_interval = ps.base_interval;
  rs.num_degrees = ps.num_degrees;
  for (int i = 0; i < ps.num_degrees; ++i) {
    rs.degree[i].voltage = ps.degree[i].voltage;
    rs.degree[i].weight = ps.degree[i].weight;
  }
  ref_xy.LoadScale(0, rs);
  tm_xy_load_scale(&port_xy, -1, &ps);
}

static GroupSettings to_ref(const tm_group& g) {
  GroupSettings r;
  r.control_mode = static_cast<ControlMode>(g.control_mode);
  r.voltage_range = static_cast<VoltageRange>(g.voltage_range);
  r.register_mode = false;
  r.register_value = 0.0f;
  r.spread = g.spread;
  r.bias = g.bias;
  r.steps = g.steps;
  r.deja_vu = g.deja_vu;
  r.scale_index = 0;
  r.length = g.length;
  r.ratio.p = g.ratio.p;
  r.ratio.q = g.ratio.q;
  return r;
}

static bool same(const void* a, const void* b, size_t n) { return memcmp(a, b, n) == 0; }

static int run_scenario(int index, uint32_t seed, int blocks) {
  ref_generator.Init(seed);
  ref_stream.Init(&ref_generator);
  ref_t.Init(&ref_stream, kSampleRate);
  ref_xy.Init(&ref_stream, kSampleRate);
  tm_rng_init(&port_rng, seed);
  tm_t_init(&port_t, &port_rng, kSampleRate);
  tm_xy_init(&port_xy, &port_rng);

  Settings s;
  randomize(&s);
  apply(s);
  load_scale(s.scale_kind);

  static float r_ext[kMaxBlock], r_master[kMaxBlock], r_s0[kMaxBlock], r_s1[kMaxBlock], r_out[kMaxBlock * 4];
  static bool r_gate[kMaxBlock * 2];
  static float p_y[kMaxBlock], p_master[kMaxBlock], p_s0[kMaxBlock], p_s1[kMaxBlock], p_out[kMaxBlock * 4];
  static bool p_gate[kMaxBlock * 2], p_tick[kMaxBlock * 4];
  static stmlib::GateFlags clock[kMaxBlock];
  memset(clock, 0, sizeof clock);

  long notes = 0;
  for (int b = 0; b < blocks; ++b) {
    if (irnd(200) == 0) {            // occasional parameter change, as a player turning knobs
      int what = irnd(4);
      if (what == 0) { randomize(&s); apply(s); load_scale(s.scale_kind); }
      else if (what == 1) { s.bias = pick_unit(); s.jitter = pick_unit(); apply(s); }
      else if (what == 2) { s.x.deja_vu = pick_unit(); s.x.length = 1 + irnd(16); s.deja_vu = pick_unit(); apply(s); }
      else { s.x.steps = pick_unit(); s.x.spread = pick_unit(); }
    }
    bool reset = irnd(1000) == 0;
    bool r_reset = reset, p_reset = reset;
    size_t size = (b & 1) ? 8 : (size_t)(1 + irnd(kMaxBlock));

    Ramps ramps;
    ramps.external = r_ext;
    ramps.master = r_master;
    ramps.slave[0] = r_s0;
    ramps.slave[1] = r_s1;
    ref_t.Process(false, &r_reset, clock, ramps, r_gate, size);
    GroupSettings rx = to_ref(s.x), ry = to_ref(s.y);
    ref_xy.Process(static_cast<ClockSource>(s.clock_source), rx, ry, &r_reset, clock, ramps, r_out, size);

    tm_t_process(&port_t, &p_reset, p_master, p_s0, p_s1, p_gate, size);
    tm_xy_process(&port_xy, s.clock_source, &s.x, &s.y, &p_reset, p_master, p_s0, p_s1, p_y, p_out, p_tick,
                  size);

    const char* bad = NULL;
    if (!same(r_master, p_master, size * sizeof(float))) bad = "master ramp";
    else if (!same(r_s0, p_s0, size * sizeof(float))) bad = "T1 ramp";
    else if (!same(r_s1, p_s1, size * sizeof(float))) bad = "T3 ramp";
    else if (!same(r_gate, p_gate, size * 2 * sizeof(bool))) bad = "gates";
    else if (!same(r_ext, p_y, size * sizeof(float))) bad = "Y ramp";
    else if (!same(r_out, p_out, size * 4 * sizeof(float))) bad = "X/Y voltages";
    if (bad) {
      printf("FAIL scenario %d (seed %u) block %d: %s differ (model %d clock %d)\n", index, seed, b, bad,
             s.model, s.clock_source);
      if (getenv("VERBOSE"))
        for (size_t i = 0; i < size; ++i)
          printf("   %2zu T1 %.9g steps %.9g ref X1 %.9g port X1 %.9g tick %d\n", i, r_s0[i], s.x.steps,
                 r_out[i * 4], p_out[i * 4], p_tick[i * 4]);
      int shown = 0;
      for (size_t i = 0; i < size * 4 && shown < 4; ++i)
        if (memcmp(&r_out[i], &p_out[i], sizeof(float)))
          printf("   sample %zu channel %zu: ref %.9g port %.9g\n", i / 4, i % 4, r_out[i], p_out[i]), shown++;
      return 1;
    }
    for (size_t i = 0; i < size * 4; ++i) notes += p_tick[i];
  }
  (void)notes;      // a very slow clock (rate -48, range 0.25x) may not tick within a scenario
  return 0;
}

int main(int argc, char** argv) {
  int scenarios = argc > 1 ? atoi(argv[1]) : 300;
  int fails = 0;
  int only = argc > 2 ? atoi(argv[2]) : -1;    // run up to SCENARIOS, report only this one
  for (int i = 0; i < scenarios && fails < 5; ++i) {
    int f = run_scenario(i, 0x1234567u + 7919u * (uint32_t)i, 4000);
    if (only < 0 || only == i)
      fails += f;
  }
  printf("tuna_marbles vs Marbles: %d scenarios x 4000 blocks, %d failed\n", scenarios, fails);
  return fails ? 1 : 0;
}
