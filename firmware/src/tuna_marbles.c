/* SPDX-License-Identifier: MIT */
/* TUNA's C port of Mutable Instruments Marbles' random section. See tuna_marbles.h.
 * Original code Copyright 2015 Emilie Gillet, MIT License. Port: the structure, names and the
 * order of every floating-point operation follow the original, so results are bit-exact. */
#include "tuna_sys.h"
#include "tuna_marbles.h"
#include "tuna_marbles_tables.h"

#define CONSTRAIN(x, lo, hi) do { if ((x) < (lo)) (x) = (lo); else if ((x) > (hi)) (x) = (hi); } while (0)

static const float kMaxUint32 = 4294967296.0f;
static const float kMaxRampValue = 0.9999f;

/* ---- stmlib helpers ------------------------------------------------------------------------------ */
static inline float interpolate(const float *table, float index, float size)
{
    index *= size;
    int32_t i = (int32_t)index;
    float f = index - (float)i;
    float a = table[i];
    float b = table[i + 1];
    return a + (b - a) * f;
}

static inline float semitones_to_ratio(float semitones)
{
    float pitch = semitones + 128.0f;
    int32_t i = (int32_t)pitch;
    float f = pitch - (float)i;
    return tm_lut_pitch_ratio_high[i] * tm_lut_pitch_ratio_low[(int32_t)(f * 256.0f)];
}

static inline float crossfade(float a, float b, float fade) { return a + (b - a) * fade; }

static void hq2_init(tm_hyst_quantizer2 *h, int num_steps, float hysteresis, bool symmetric)
{
    h->num_steps = num_steps;
    h->hysteresis = hysteresis;
    h->scale = (float)(symmetric ? num_steps - 1 : num_steps);
    h->offset = symmetric ? 0.0f : -0.5f;
    h->quantized_value = 0;
}

static int hq2_process(tm_hyst_quantizer2 *h, float value)
{
    value *= h->scale;
    value += h->offset;
    value += (float)0;
    float sign = value > (float)h->quantized_value ? -1.0f : +1.0f;
    int q = (int)(value + sign * h->hysteresis + 0.5f);
    CONSTRAIN(q, 0, h->num_steps - 1);
    h->quantized_value = q;
    return q;
}

static int hq_process(tm_hyst_quantizer *h, float value, int num_steps)
{
    const float hysteresis = 0.25f;
    value *= (float)(num_steps - 1);
    value += (float)0;
    float feedback = value > (float)h->quantized_value ? -hysteresis : hysteresis;
    int q = (int)(value + feedback + 0.5f);
    CONSTRAIN(q, 0, num_steps - 1);
    h->quantized_value = q;
    return q;
}

/* ---- distributions.h ------------------------------------------------------------------------------ */
enum { kNumBiasValues = 5, kNumRangeValues = 9 };
static const float kIcdfTableSize = 128.0f;

static float beta_distribution_sample(float uniform, float spread, float bias)
{
    bool flip_result = bias > 0.5f;
    if (flip_result) {
        uniform = 1.0f - uniform;
        bias = 1.0f - bias;
    }
    bias *= ((float)kNumBiasValues - 1.0f) * 2.0f;
    spread *= ((float)kNumRangeValues - 1.0f);
    int32_t bias_integral = (int32_t)bias;
    float bias_fractional = bias - (float)bias_integral;
    int32_t spread_integral = (int32_t)spread;
    float spread_fractional = spread - (float)spread_integral;
    size_t cell = (size_t)bias_integral * (kNumRangeValues + 1) + (size_t)spread_integral;
    size_t offset = 0;
    if (uniform <= 0.05f) {
        offset = (size_t)kIcdfTableSize + 1;
        uniform *= 20.0f;
    } else if (uniform >= 0.95f) {
        offset = 2 * ((size_t)kIcdfTableSize + 1);
        uniform = (uniform - 0.95f) * 20.0f;
    }
    const float *const *t = tm_distributions_table;
    float x1y1 = interpolate(t[cell] + offset, uniform, kIcdfTableSize);
    float x2y1 = interpolate(t[cell + 1] + offset, uniform, kIcdfTableSize);
    float x1y2 = interpolate(t[cell + kNumRangeValues + 1] + offset, uniform, kIcdfTableSize);
    float x2y2 = interpolate(t[cell + kNumRangeValues + 2] + offset, uniform, kIcdfTableSize);
    float y1 = x1y1 + (x2y1 - x1y1) * spread_fractional;
    float y2 = x1y2 + (x2y2 - x1y2) * spread_fractional;
    float y = y1 + (y2 - y1) * bias_fractional;
    if (flip_result)
        y = 1.0f - y;
    return y;
}

static inline float fast_beta_distribution_sample(float uniform)
{
    return interpolate(tm_dist_icdf_4_3, uniform, kIcdfTableSize);
}

/* ---- random stream / sequence ---------------------------------------------------------------------- */
void tm_rng_init(tm_rng *r, uint32_t seed) { r->state = seed; }

static inline uint32_t rng_word(tm_rng *r)
{
    r->state = r->state * 1664525u + 1013904223u;
    return r->state;
}

float tm_rng_float(tm_rng *r) { return (float)rng_word(r) / 4294967296.0f; }

void tm_seq_init(tm_sequence *s, tm_rng *rng)
{
    s->rng = rng;
    for (int i = 0; i < TM_DEJA_VU_BUFFER; ++i)
        s->loop[i] = tm_rng_float(rng);
    for (int i = 0; i < TM_HISTORY_BUFFER; ++i)
        s->history[i] = 0.0f;
    s->loop_write_head = 0;
    s->length = 8;
    s->step = 0;
    s->record_head = 0;
    s->replay_head = -1;
    s->replay_start = 0;
    s->deja_vu = 0.0f;
    s->replay_hash = s->replay_shift = 0;
    s->redo_read = 0;
    s->redo_write = -1;
    s->redo_write_history = -1;
}

static void seq_clone(tm_sequence *d, const tm_sequence *src)
{
    *d = *src;      /* indices instead of pointers: a plain copy is the original's Clone() */
}

static inline void seq_record(tm_sequence *s)
{
    s->replay_start = s->record_head;
    s->replay_head = -1;
}

static inline void seq_replay_pseudo_random(tm_sequence *s, uint32_t hash)
{
    s->replay_head = s->replay_start;
    s->replay_hash = hash;
    s->replay_shift = 0;
}

static inline void seq_replay_shifted(tm_sequence *s, uint32_t shift)
{
    s->replay_head = s->replay_start;
    s->replay_hash = 0;
    s->replay_shift = shift;
}

static inline float seq_get_replay_value(const tm_sequence *s)
{
    uint32_t h = ((uint32_t)s->replay_head - 1 - s->replay_shift + 2 * TM_HISTORY_BUFFER) % TM_HISTORY_BUFFER;
    if (!s->replay_hash)
        return s->history[h];
    uint32_t word = (uint32_t)(float)(s->history[h] * kMaxUint32);
    word = (word ^ s->replay_hash) * 1664525u + 1013904223u;
    return (float)word / kMaxUint32;
}

float tm_seq_next_value(tm_sequence *s, bool deterministic, float value)
{
    if (s->replay_head >= 0) {
        s->replay_head = (s->replay_head + 1) % TM_HISTORY_BUFFER;
        return seq_get_replay_value(s);
    }
    const float p_sqrt = 2.0f * s->deja_vu - 1.0f;
    const float p = p_sqrt * p_sqrt;
    const bool mutate = tm_rng_float(s->rng) < p;
    if (mutate && s->deja_vu <= 0.5f) {
        s->redo_write = s->loop_write_head;
        s->loop[s->redo_write] = deterministic ? 1.0f + value : tm_rng_float(s->rng);
        s->loop_write_head = (s->loop_write_head + 1) % TM_DEJA_VU_BUFFER;
        s->step = s->length - 1;
    } else {
        s->redo_write = -1;
        if (mutate) {
            s->step = (int)(tm_rng_float(s->rng) * (float)s->length);
        } else {
            s->step = s->step + 1;
            if (s->step >= s->length)
                s->step = 0;
        }
    }
    uint32_t i = (uint32_t)(s->loop_write_head + TM_DEJA_VU_BUFFER - s->length + s->step);
    s->redo_read = (int)(i % TM_DEJA_VU_BUFFER);
    float result = s->loop[s->redo_read];
    if (result >= 1.0f)
        result -= 1.0f;
    else if (deterministic)
        result = 0.5f;
    s->redo_write_history = s->record_head;
    s->history[s->redo_write_history] = result;
    s->record_head = (s->record_head + 1) % TM_HISTORY_BUFFER;
    return result;
}

void tm_seq_next_vector(tm_sequence *s, float *dst, size_t n)
{
    float seed = tm_seq_next_value(s, false, 0.0f);
    uint32_t word = (uint32_t)(float)(seed * kMaxUint32);
    while (n--) {
        *dst++ = (float)word / kMaxUint32;
        word = word * 1664525u + 1013904223u;
    }
}

void tm_seq_set_length(tm_sequence *s, int length)
{
    if (length < 1 || length > TM_DEJA_VU_BUFFER)
        return;
    s->length = length;
    s->step = s->step % length;
}

void tm_seq_reset(tm_sequence *s) { s->step = s->length - 1; }

/* ---- slave_ramp.h ---------------------------------------------------------------------------------- */
static void sr_init(tm_slave_ramp *r)
{
    r->phase = 0.0f;
    r->max_phase = kMaxRampValue;
    r->ratio = 1.0f;
    r->pulse_width = 0.0f;
    r->target = 1.0f;
    r->pulse_length = 0;
    r->bernoulli = false;
    r->must_complete = false;
}

static void sr_reset(tm_slave_ramp *r)
{
    sr_init(r);
    r->phase = 1.0f;
}

static void sr_init_divider(tm_slave_ramp *r, int pattern_length, tm_ratio ratio, float pulse_width)
{
    r->bernoulli = false;
    r->phase = 0.0f;
    r->max_phase = (float)pattern_length * kMaxRampValue;
    r->ratio = (float)ratio.p / (float)ratio.q;
    r->pulse_width = pulse_width;
    r->target = 1.0f;
    r->pulse_length = 0;
}

static void sr_init_bernoulli(tm_slave_ramp *r, bool must_complete, float pulse_width, float expected_value)
{
    r->bernoulli = true;
    if (r->must_complete) {
        r->phase = 0.0f;
        r->pulse_width = pulse_width;
        r->ratio = 1.0f;
        r->pulse_length = 0;
    }
    if (!must_complete)
        r->ratio = (1.0f - r->phase) * expected_value;
    else
        r->ratio = 1.0f - r->phase;
    r->must_complete = must_complete;
}

static inline void sr_process(tm_slave_ramp *r, float frequency, float *phase, bool *gate)
{
    float output_phase;
    if (r->bernoulli) {
        r->phase += frequency * r->ratio;
        output_phase = r->phase;
        if (output_phase >= 1.0f)
            output_phase = 1.0f;
    } else {
        r->phase += frequency;
        if (r->phase >= r->max_phase)
            r->phase = r->max_phase;
        output_phase = r->phase * r->ratio;
        if (output_phase > r->target) {
            r->pulse_length = 0;
            r->target += 1.0f;
        }
        output_phase -= (float)(int)output_phase;
    }
    *phase = output_phase;
    *gate = r->pulse_width == 0.0f ? r->pulse_length < 32 && output_phase <= 0.5f
                                   : output_phase < r->pulse_width;
    ++r->pulse_length;
}

/* ---- t_generator.cc -------------------------------------------------------------------------------- */
typedef struct { tm_ratio ratios[TM_T_CHANNELS]; int32_t length; } divider_pattern;

#define NUM_DIVIDER_PATTERNS 17
#define NUM_DRUM_PATTERNS 18
#define DRUM_PATTERN_SIZE 8

static const divider_pattern divider_patterns[NUM_DIVIDER_PATTERNS] = {
    {{{1, 1}, {1, 1}}, 1}, {{{1, 1}, {2, 1}}, 1}, {{{1, 2}, {1, 1}}, 2}, {{{1, 1}, {4, 1}}, 1},
    {{{1, 2}, {2, 1}}, 2}, {{{1, 1}, {3, 2}}, 2}, {{{1, 4}, {4, 1}}, 4}, {{{1, 4}, {2, 1}}, 4},
    {{{1, 2}, {3, 2}}, 2}, {{{1, 1}, {8, 1}}, 1}, {{{1, 1}, {3, 1}}, 1}, {{{1, 3}, {1, 1}}, 3},
    {{{1, 1}, {5, 4}}, 4}, {{{1, 2}, {5, 4}}, 4}, {{{1, 1}, {6, 1}}, 1}, {{{1, 3}, {2, 1}}, 3},
    {{{1, 1}, {16, 1}}, 1},
};

static const divider_pattern fixed_divider_patterns[NUM_DIVIDER_PATTERNS] = {
    {{{8, 1}, {1, 8}}, 8},  {{{6, 1}, {1, 6}}, 6},   {{{4, 1}, {1, 4}}, 4},   {{{3, 1}, {1, 3}}, 3},
    {{{2, 1}, {1, 2}}, 2},  {{{3, 2}, {2, 3}}, 6},   {{{4, 3}, {3, 4}}, 12},  {{{5, 4}, {4, 5}}, 20},
    {{{1, 1}, {1, 1}}, 1},  {{{4, 5}, {5, 4}}, 20},  {{{3, 4}, {4, 3}}, 12},  {{{2, 2}, {3, 2}}, 6},
    {{{1, 2}, {2, 1}}, 2},  {{{1, 3}, {3, 1}}, 3},   {{{1, 4}, {4, 1}}, 4},   {{{1, 6}, {6, 1}}, 6},
    {{{1, 8}, {8, 1}}, 8},
};

static const uint8_t drum_patterns[NUM_DRUM_PATTERNS][DRUM_PATTERN_SIZE] = {
    {1, 0, 0, 0, 2, 0, 0, 0}, {0, 0, 1, 0, 2, 0, 0, 0}, {1, 0, 1, 0, 2, 0, 0, 0},
    {0, 0, 1, 0, 2, 0, 0, 2}, {1, 0, 1, 0, 2, 0, 1, 0}, {0, 2, 1, 0, 2, 0, 0, 2},
    {1, 0, 0, 0, 2, 0, 1, 0}, {0, 2, 1, 0, 2, 0, 1, 2}, {1, 0, 0, 1, 2, 0, 0, 0},
    {0, 2, 1, 1, 2, 0, 1, 2}, {1, 0, 0, 1, 2, 0, 1, 0}, {0, 2, 1, 1, 2, 2, 1, 2},
    {1, 0, 0, 1, 2, 0, 1, 2}, {0, 2, 0, 1, 2, 0, 1, 2}, {1, 0, 1, 1, 2, 0, 1, 2},
    {2, 0, 1, 2, 0, 1, 2, 0}, {1, 2, 1, 1, 2, 0, 1, 2}, {2, 0, 1, 2, 0, 1, 2, 2},
};

/* RandomVector: pulse_width[2], u[2], p, jitter */
enum { RV_PW0, RV_PW1, RV_U0, RV_U1, RV_P, RV_JITTER, RV_SIZE };

void tm_t_init(tm_tgen *t, tm_rng *rng, float sr)
{
    t->one_hertz = 1.0f / sr;
    t->model = TM_T_COMPLEMENTARY_BERNOULLI;
    t->range = TM_T_RANGE_1X;
    t->rate = 0.0f;
    t->bias = 0.5f;
    t->jitter = 0.0f;
    t->pulse_width_mean = 0.0f;
    t->pulse_width_std = 0.0f;
    t->rate_hz = 0.0f;
    t->master_phase = 0.0f;
    t->jitter_multiplier = 1.0f;
    t->phase_difference = 0.0f;
    t->divider_pattern_length = 0;
    for (int i = 0; i < TM_MARKOV_HISTORY; ++i)
        t->streak_counter[i] = t->markov_history[i] = 0;
    t->markov_history_ptr = 0;
    t->drum_pattern_step = 0;
    t->drum_pattern_index = 0;
    tm_seq_init(&t->sequence, rng);
    for (int i = 0; i < TM_T_CHANNELS; ++i)
        sr_init(&t->slave_ramp[i]);
    hq2_init(&t->bias_quantizer, NUM_DIVIDER_PATTERNS, 0.1f, false);
}

static float random_pulse_width(const tm_tgen *t, float u)
{
    if (t->pulse_width_std == 0.0f)
        return 0.05f + 0.9f * t->pulse_width_mean;
    return 0.05f + 0.9f * beta_distribution_sample(u, t->pulse_width_std, t->pulse_width_mean);
}

static int gen_complementary_bernoulli(const tm_tgen *t, const float *x)
{
    int bitmask = 0;
    for (int i = 0; i < TM_T_CHANNELS; ++i)
        if ((x[RV_U0 + (i >> 1)] > t->bias) ^ (i & 1))
            bitmask |= 1 << i;
    return bitmask;
}

static int gen_independent_bernoulli(const tm_tgen *t, const float *x)
{
    int bitmask = 0;
    for (int i = 0; i < TM_T_CHANNELS; ++i)
        if ((x[RV_U0 + i] > t->bias) ^ (i & 1))
            bitmask |= 1 << i;
    return bitmask;
}

static int gen_three_states(const tm_tgen *t, const float *x)
{
    int bitmask = 0;
    float p_none = 0.75f - fabsf(t->bias - 0.5f);
    float threshold = p_none + (1.0f - p_none) * (0.25f + (t->bias * 0.5f));
    for (int i = 0; i < TM_T_CHANNELS; ++i) {
        float u = x[RV_U0 + (i >> 1)];
        if (u > p_none && ((u > threshold) ^ (i & 1)))
            bitmask |= 1 << i;
    }
    return bitmask;
}

static int gen_drums(tm_tgen *t, const float *x)
{
    ++t->drum_pattern_step;
    if (t->drum_pattern_step >= DRUM_PATTERN_SIZE) {
        t->drum_pattern_step = 0;
        float u = x[RV_U0] * 2.0f * fabsf(t->bias - 0.5f);
        t->drum_pattern_index = (size_t)(int32_t)(NUM_DRUM_PATTERNS * u);
        if (t->bias <= 0.5f)
            t->drum_pattern_index -= t->drum_pattern_index % 2;
    }
    return drum_patterns[t->drum_pattern_index][t->drum_pattern_step];
}

static int gen_markov(tm_tgen *t, const float *x)
{
    int bitmask = 0;
    float b = 1.5f * t->bias - 0.5f;
    t->markov_history[t->markov_history_ptr] = 0;
    const int32_t p = t->markov_history_ptr;
    for (int i = 0; i < TM_T_CHANNELS; ++i) {
        int32_t mask = 1 << i;
        /* favour repeating 8 ticks ago, avoid hits on both channels, favour sparse patterns,
         * favour one channel echoing the other 4 ticks later */
        bool periodic = t->markov_history[(p + 8) % TM_MARKOV_HISTORY] & mask;
        bool simultaneous = t->markov_history[(p + 8) % TM_MARKOV_HISTORY] & ~mask;
        bool dense = t->markov_history[(p + 1) % TM_MARKOV_HISTORY] & mask;
        bool alternate = t->markov_history[(p + 4) % TM_MARKOV_HISTORY] & ~mask;
        float logit = -1.5f;
        logit += t->streak_counter[i] > 24 ? 10.0f : 0.0f;
        logit += 8.0f * fabsf(b) * (periodic ? b : -b);
        logit -= 2.0f * (simultaneous ? b : -b);
        logit -= 1.0f * (dense ? b : 0.0f);
        logit += 1.0f * (alternate ? b : 0.0f);
        CONSTRAIN(logit, -10.0f, 10.0f);
        float probability = tm_lut_logit[(int)(logit * 12.8f + 128.0f)];
        bool state = x[RV_U0 + i] < probability;
        if (t->sequence.deja_vu >= x[RV_P])
            state = t->markov_history[(p + t->sequence.length) % TM_MARKOV_HISTORY] & mask;
        if (state) {
            bitmask |= mask;
            t->streak_counter[i] = 0;
        } else {
            ++t->streak_counter[i];
        }
    }
    t->markov_history[p] |= bitmask;
    t->markov_history_ptr = (p + TM_MARKOV_HISTORY - 1) % TM_MARKOV_HISTORY;
    return bitmask;
}

static void schedule_output_pulses(tm_tgen *t, const float *x, int bitmask)
{
    for (int i = 0; i < TM_T_CHANNELS; ++i) {
        sr_init_bernoulli(&t->slave_ramp[i], bitmask & 1, random_pulse_width(t, x[RV_PW0 + i]), 0.5f);
        bitmask >>= 1;
    }
}

static void configure_slave_ramps(tm_tgen *t, const float *x)
{
    switch (t->model) {
    case TM_T_COMPLEMENTARY_BERNOULLI: schedule_output_pulses(t, x, gen_complementary_bernoulli(t, x)); break;
    case TM_T_INDEPENDENT_BERNOULLI:   schedule_output_pulses(t, x, gen_independent_bernoulli(t, x)); break;
    case TM_T_THREE_STATES:            schedule_output_pulses(t, x, gen_three_states(t, x)); break;
    case TM_T_DRUMS:                   schedule_output_pulses(t, x, gen_drums(t, x)); break;
    case TM_T_MARKOV:                  schedule_output_pulses(t, x, gen_markov(t, x)); break;
    case TM_T_CLUSTERS:
    case TM_T_DIVIDER:
        --t->divider_pattern_length;
        if (t->divider_pattern_length <= 0) {
            divider_pattern pattern;
            if (t->model == TM_T_DIVIDER) {
                pattern = fixed_divider_patterns[hq2_process(&t->bias_quantizer, t->bias)];
            } else {
                float strength = fabsf(t->bias - 0.5f) * 2.0f;
                float u = x[RV_U0];
                u *= (u + strength * strength * (1.0f - u));
                u *= strength;
                pattern = divider_patterns[(size_t)(u * NUM_DIVIDER_PATTERNS)];
                if (t->bias < 0.5f) {
                    for (int i = 0; i < TM_T_CHANNELS / 2; ++i) {
                        tm_ratio tmp = pattern.ratios[i];
                        pattern.ratios[i] = pattern.ratios[TM_T_CHANNELS - 1 - i];
                        pattern.ratios[TM_T_CHANNELS - 1 - i] = tmp;
                    }
                }
            }
            for (int i = 0; i < TM_T_CHANNELS; ++i)
                sr_init_divider(&t->slave_ramp[i], pattern.length, pattern.ratios[i],
                                random_pulse_width(t, x[RV_PW0 + i]));
            t->divider_pattern_length = pattern.length;
        }
        break;
    }
}

void tm_t_process(tm_tgen *t, bool *reset, float *master, float *slave0, float *slave1, bool *gate, size_t size)
{
    float rate = 2.0f;
    if (t->range == TM_T_RANGE_4X)
        rate = 8.0f;
    else if (t->range == TM_T_RANGE_0_25X)
        rate = 0.5f;
    float internal_frequency = t->rate_hz > 0.0f ? t->rate_hz * t->one_hertz
                                                 : rate * t->one_hertz * semitones_to_ratio(t->rate);

    if (reset && *reset) {
        for (int i = 0; i < TM_T_CHANNELS; ++i)
            sr_reset(&t->slave_ramp[i]);
        tm_seq_reset(&t->sequence);
        t->divider_pattern_length = 0;
        t->drum_pattern_step = DRUM_PATTERN_SIZE;
        if (t->model != TM_T_DIVIDER) {
            float v[RV_SIZE];
            tm_seq_next_vector(&t->sequence, v, RV_SIZE);
            configure_slave_ramps(t, v);
        }
    }

    float *slave[TM_T_CHANNELS] = {slave0, slave1};
    while (size--) {
        float frequency = internal_frequency;
        frequency += frequency < 0.0f ? 1.0f : 0.0f;
        float jittery_frequency = frequency * t->jitter_multiplier;
        t->master_phase += jittery_frequency;
        t->phase_difference += frequency - jittery_frequency;
        if (t->master_phase > 1.0f) {
            t->master_phase -= 1.0f;
            float v[RV_SIZE];
            tm_seq_next_vector(&t->sequence, v, RV_SIZE);
            float jitter_amount = t->jitter * t->jitter * t->jitter * t->jitter * 36.0f;
            float x = fast_beta_distribution_sample(v[RV_JITTER]);
            float multiplier = semitones_to_ratio((x * 2.0f - 1.0f) * jitter_amount);
            /* keep the jittery clock close to the straight one: the further it drifts, the more
             * likely it speeds up or slows down to catch up */
            multiplier *= t->phase_difference > 0.0f ? 1.0f + t->phase_difference
                                                     : 1.0f / (1.0f - t->phase_difference);
            t->jitter_multiplier = multiplier;
            configure_slave_ramps(t, v);
        }
        *master++ = t->master_phase;
        for (int j = 0; j < TM_T_CHANNELS; ++j) {
            sr_process(&t->slave_ramp[j], frequency * t->jitter_multiplier, slave[j], gate);
            slave[j]++;
            gate++;
        }
    }
}

/* ---- quantizer.cc ---------------------------------------------------------------------------------- */
void tm_scale_init_major(tm_scale *s)
{
    static const uint8_t w[12] = {255, 16, 128, 16, 192, 64, 8, 224, 16, 96, 32, 160};
    s->base_interval = 1.0f;
    s->num_degrees = 12;
    for (int i = 0; i < 12; ++i) {
        s->degree[i].voltage = (float)i * 0.0833333333f;
        s->degree[i].weight = w[i];
    }
}

void tm_scale_from_mask(tm_scale *s, uint16_t mask)
{
    /* semitones in order of importance, and the weights the n-th scale note gets */
    static const uint8_t order[12] = {0, 7, 4, 3, 11, 10, 2, 1, 9, 8, 5, 6};
    static const uint8_t rank_weight[7] = {255, 224, 192, 160, 128, 96, 64};
    s->base_interval = 1.0f;
    s->num_degrees = 12;
    for (int i = 0; i < 12; ++i) {
        s->degree[i].voltage = (float)i * 0.0833333333f;
        s->degree[i].weight = 16;           /* outside the scale: only at STEPS' chromatic end */
    }
    int rank = 0;
    for (int k = 0; k < 12; ++k) {
        int st = order[k];
        if ((mask >> st) & 1)
            s->degree[st].weight = rank_weight[rank < 6 ? rank : 6], rank++;
    }
}

void tm_quantizer_init(tm_quantizer *q, const tm_scale *scale)
{
    int n = scale->num_degrees;
    if (!n || n > TM_MAX_DEGREES || scale->base_interval == 0.0f)
        return;
    q->num_degrees = n;
    q->base_interval = scale->base_interval;
    q->base_interval_reciprocal = 1.0f / scale->base_interval;
    uint8_t second_largest_threshold = 0;
    for (int i = 0; i < n; ++i) {
        q->voltage[i] = scale->degree[i].voltage;
        if (scale->degree[i].weight != 255 && scale->degree[i].weight >= second_largest_threshold)
            second_largest_threshold = scale->degree[i].weight;
    }
    uint8_t thresholds[TM_NUM_THRESHOLDS] = {0, 16, 32, 64, 128, 192, 255};
    if (second_largest_threshold > 192)
        thresholds[TM_NUM_THRESHOLDS - 2] = second_largest_threshold;
    for (int t = 0; t < TM_NUM_THRESHOLDS; ++t) {
        uint16_t bitmask = 0;
        uint8_t first = 0xff, last = 0;
        for (int i = 0; i < n; ++i) {
            if (scale->degree[i].weight >= thresholds[t]) {
                bitmask |= (uint16_t)(1 << i);
                if (first == 0xff)
                    first = (uint8_t)i;
                last = (uint8_t)i;
            }
        }
        q->level[t].bitmask = bitmask;
        q->level[t].first = first;
        q->level[t].last = last;
    }
    q->level_quantizer.quantized_value = 0;
    for (int t = 0; t < TM_NUM_THRESHOLDS; ++t)
        q->feedback[t] = 0.0f;
}

float tm_quantizer_process(tm_quantizer *q, float value, float amount, bool hysteresis)
{
    int level = hq_process(&q->level_quantizer, amount, TM_NUM_THRESHOLDS + 1);
    float quantized_voltage = value;
    if (level > 0) {
        level -= 1;
        float raw_value = value;
        if (hysteresis)
            value += q->feedback[level];
        const float note = value * q->base_interval_reciprocal;
        int32_t note_integral = (int32_t)note;
        float note_fractional = note - (float)note_integral;
        if (value < 0.0f) {
            note_integral -= 1;
            note_fractional += 1.0f;
        }
        note_fractional *= q->base_interval;
        float a = q->voltage[q->level[level].last] - q->base_interval;
        float b = q->voltage[q->level[level].first] + q->base_interval;
        uint16_t bitmask = q->level[level].bitmask;
        for (int i = 0; i < q->num_degrees; ++i) {
            if (bitmask & 1) {
                float v = q->voltage[i];
                if (note_fractional > v) {
                    a = v;
                } else {
                    b = v;
                    break;
                }
            }
            bitmask >>= 1;
        }
        quantized_voltage = note_fractional < (a + b) * 0.5f ? a : b;
        quantized_voltage += (float)note_integral * q->base_interval;
        q->feedback[level] = (quantized_voltage - raw_value) * 0.25f;
    }
    return quantized_voltage;
}

/* ---- lag_processor.cc ------------------------------------------------------------------------------ */
static void lag_init(tm_lag *l) { l->ramp_start = l->ramp_value = l->lp_state = l->previous_phase = 0.0f; }

static float lag_process(tm_lag *l, float value, float smoothness, float phase)
{
    float frequency = phase - l->previous_phase;
    if (frequency < 0.0f)
        frequency += 1.0f;
    l->previous_phase = phase;
    frequency *= 0.25f;
    frequency *= semitones_to_ratio(84.0f * (1.0f - smoothness));
    if (frequency >= 1.0f)
        frequency = 1.0f;
    if (smoothness <= 0.05f)
        frequency += 20.f * (0.05f - smoothness) * (1.0f - frequency);
    l->lp_state += (value - l->lp_state) * frequency;
    float interp_amount = (smoothness - 0.6f) * 5.0f;
    CONSTRAIN(interp_amount, 0.0f, 1.0f);
    float interp_linearity = (1.0f - smoothness) * 5.0f;
    CONSTRAIN(interp_linearity, 0.0f, 1.0f);
    /* Deviation (memory safety): with extreme jitter a Bernoulli slave ramp can run below 0 (its
     * ratio (1 - phase) * 0.5 turns negative once phase passed 1), and the original then reads
     * before lut_raised_cosine. The reference test applies the same clamp to the original. */
    float table_phase = phase < 0.0f ? 0.0f : phase > 1.0f ? 1.0f : phase;
    float warped_phase = interpolate(tm_lut_raised_cosine, table_phase, 256.0f);
    float interp_phase = crossfade(warped_phase, phase, interp_linearity);
    float interp = crossfade(l->ramp_start, value, interp_phase);
    l->ramp_value = interp;
    return crossfade(l->lp_state, interp, interp_amount);
}

/* ---- output_channel.cc ----------------------------------------------------------------------------- */
static void oc_init(tm_output_channel *c)
{
    c->spread = 0.5f;
    c->bias = 0.5f;
    c->steps = 0.5f;
    c->previous_steps = 0.0f;
    c->previous_phase = 0.0f;
    c->previous_voltage = 0.0f;
    c->voltage = 0.0f;
    c->quantized_voltage = 0.0f;
    c->scale = 10.0f;
    c->offset = -5.0f;
    lag_init(&c->lag);
    tm_scale s = {1.0f, 1, {{0.0f, 0}}};       /* Scale::Init(): one degree, no quantization */
    tm_quantizer_init(&c->quantizer, &s);
}

static float oc_generate_new_voltage(tm_output_channel *c, tm_sequence *seq)
{
    float u = tm_seq_next_value(seq, false, 0.0f);
    float degenerate_amount = 1.25f - c->spread * 25.0f;
    float bernoulli_amount = c->spread * 25.0f - 23.75f;
    CONSTRAIN(degenerate_amount, 0.0f, 1.0f);
    CONSTRAIN(bernoulli_amount, 0.0f, 1.0f);
    float value = beta_distribution_sample(u, c->spread, c->bias);
    float bernoulli_value = u >= (1.0f - c->bias) ? 0.999999f : 0.0f;
    value += degenerate_amount * (c->bias - value);
    value += bernoulli_amount * (bernoulli_value - value);
    return value * c->scale + c->offset;
}

static void oc_process(tm_output_channel *c, tm_sequence *seq, const float *phase, float *output, bool *tick,
                       size_t size, size_t stride)
{
    /* stmlib::ParameterInterpolator over the block */
    float steps_value = c->previous_steps;
    float steps_increment = (c->steps - c->previous_steps) / (float)size;
    while (size--) {
        steps_value += steps_increment;
        const float steps = steps_value;
        bool new_value = *phase < c->previous_phase;
        if (new_value) {
            c->previous_voltage = c->voltage;
            c->voltage = oc_generate_new_voltage(c, seq);
            c->lag.ramp_start = c->lag.ramp_value;
            c->quantized_voltage = tm_quantizer_process(&c->quantizer, c->voltage, 2.0f * steps - 1.0f, false);
        }
        if (steps >= 0.5f) {
            *output = c->quantized_voltage;
        } else {
            const float smoothness = 1.0f - 2.0f * steps;
            *output = lag_process(&c->lag, c->voltage, smoothness, *phase);
        }
        if (tick) {
            *tick = new_value;
            tick += stride;
        }
        output += stride;
        c->previous_phase = *phase++;
    }
    c->previous_steps = steps_value;
}

/* ---- ramp_divider.h -------------------------------------------------------------------------------- */
static void rd_init(tm_ramp_divider *d)
{
    d->phase = 0.0f;
    d->train_phase = 0.0f;
    d->max_train_phase = 1.0f;
    d->f_ratio = 0.99999f;
    d->reset_counter = 1;
    d->reset_at_next_pulse = false;
}

static void rd_process(tm_ramp_divider *d, tm_ratio ratio, const float *in, float *out, size_t size)
{
    while (size--) {
        float new_phase = *in++;
        float frequency = new_phase - d->phase;
        if (frequency < 0.0f) {
            if (d->reset_at_next_pulse) {
                d->reset_at_next_pulse = false;
                d->reset_counter = 1;
            }
            frequency += 1.0f;
            --d->reset_counter;
            if (!d->reset_counter) {
                d->train_phase = new_phase;
                d->reset_counter = ratio.q;
                d->f_ratio = (float)ratio.p / (float)ratio.q * kMaxRampValue;
                frequency = 0.0f;
                d->max_train_phase = (float)ratio.q;
            }
        }
        d->train_phase += frequency;
        if (d->train_phase >= d->max_train_phase)
            d->train_phase = d->max_train_phase;
        float output_phase = d->train_phase * d->f_ratio;
        output_phase -= (float)(int)output_phase;
        *out++ = output_phase;
        d->phase = new_phase;
    }
}

/* ---- x_y_generator.cc ------------------------------------------------------------------------------ */
void tm_xy_init(tm_xygen *g, tm_rng *rng)
{
    for (int i = 0; i < TM_CHANNELS; ++i) {
        tm_seq_init(&g->sequence[i], rng);
        oc_init(&g->channel[i]);
        g->use_shifted_sequences[i] = false;
    }
    rd_init(&g->ramp_divider);
}

void tm_xy_load_scale(tm_xygen *g, int channel, const tm_scale *s)
{
    for (int i = 0; i < TM_X_CHANNELS; ++i)
        if (channel < 0 || channel == i)
            tm_quantizer_init(&g->channel[i].quantizer, s);
    if (channel == TM_X_CHANNELS)
        tm_quantizer_init(&g->channel[TM_X_CHANNELS].quantizer, s);
}

static const uint32_t hashes[TM_X_CHANNELS] = {0, 0xbeca55e5, 0xf0cacc1a};

void tm_xy_process(tm_xygen *g, int clock_source, const tm_group *x, const tm_group *y, bool *reset,
                   const float *master, const float *slave0, const float *slave1, float *y_ramp,
                   float *output, bool *tick, size_t size)
{
    const float *channel_ramp[TM_CHANNELS];
    switch (clock_source) {
    case TM_CLOCK_T1: channel_ramp[0] = channel_ramp[1] = channel_ramp[2] = slave0; break;
    case TM_CLOCK_T2: channel_ramp[0] = channel_ramp[1] = channel_ramp[2] = master; break;
    case TM_CLOCK_T3: channel_ramp[0] = channel_ramp[1] = channel_ramp[2] = slave1; break;
    default:
        channel_ramp[0] = slave0;
        channel_ramp[1] = master;
        channel_ramp[2] = slave1;
        break;
    }
    bool do_reset = reset && *reset;
    if (do_reset)
        g->ramp_divider.reset_at_next_pulse = true;
    rd_process(&g->ramp_divider, y->ratio, channel_ramp[1], y_ramp, size);
    channel_ramp[TM_CHANNELS - 1] = y_ramp;

    for (int i = 0; i < TM_CHANNELS; ++i) {
        tm_output_channel *channel = &g->channel[i];
        const tm_group *settings = i < TM_X_CHANNELS ? x : y;
        switch (settings->voltage_range) {
        case TM_RANGE_NARROW:   channel->scale = 2.0f;  channel->offset = 0.0f;  break;
        case TM_RANGE_POSITIVE: channel->scale = 5.0f;  channel->offset = 0.0f;  break;
        case TM_RANGE_FULL:     channel->scale = 10.0f; channel->offset = -5.0f; break;
        default: break;
        }
        float amount = 1.0f;
        if (settings->control_mode == TM_CONTROL_BUMP)
            amount = i == TM_X_CHANNELS / 2 ? 1.0f : -1.0f;
        else if (settings->control_mode == TM_CONTROL_TILT)
            amount = 2.0f * (float)i / (float)(TM_X_CHANNELS - 1) - 1.0f;
        channel->spread = 0.5f + (settings->spread - 0.5f) * amount;
        channel->bias = 0.5f + (settings->bias - 0.5f) * amount;
        channel->steps = 0.5f + (settings->steps - 0.5f) * amount;

        tm_sequence *sequence = &g->sequence[i];
        seq_record(sequence);
        tm_seq_set_length(sequence, settings->length);
        sequence->deja_vu = settings->deja_vu;
        if (do_reset)
            tm_seq_reset(sequence);
        bool use_shifted = false;
        if (clock_source != TM_CLOCK_T1_T2_T3 && i > 0 && i < TM_X_CHANNELS) {
            sequence = &g->sequence[0];
            seq_replay_pseudo_random(sequence, hashes[i]);
        }
        if (!use_shifted && g->use_shifted_sequences[i])
            seq_clone(sequence, &g->sequence[0]);
        g->use_shifted_sequences[i] = use_shifted;
        oc_process(channel, sequence, channel_ramp[i], &output[i], tick ? &tick[i] : NULL, size, TM_CHANNELS);
    }
    (void)seq_replay_shifted;   /* register mode only; kept for parity with the original */
}
