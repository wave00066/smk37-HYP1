/* SPDX-License-Identifier: GPL-3.0-only */
/* HYP-1's FM voice (EN_FM: the SMK-37 build). Part of amb.c (included before render_pad).
 *
 * A DX7-style 6-operator engine: fm6_core.c, msfa (Dexed's core, Apache-2.0) as Felucca ported it to integers.
 * The patches: the 128 factory voices M-VAVE ships on the FM-1 and the SMK-37 (banks A-D, dx7_factory.inc,
 * Apache-2.0), and one user bank of 32 (U01-U32), received as a DX7 32-voice bulk dump over USB and stored.
 *
 *   the FM voice    FM_NHAND notes played from the keys (or MIDI channel 2), poly or mono with glide;
 *                   its own patch, level, brightness (the modulators' level), space (echo and reverb), octave
 *   the pad         the pad's Sound past its sixteen: a DX7 patch sings the pad's four voices. Each breath (or
 *                   stab, or played key) is a note-on, the breath's fall a note-off; the pad's own envelope,
 *                   pan and level ride on top, so the chords still breathe
 *
 * fm6 renders CTL (32) samples at a time; HYP-1's chunks are any length (they split at the clock and the
 * drums), so each output keeps a little FIFO of rendered samples (at most a block ahead: 0.7 ms). */
#include "fm6_tables.h"
#include "fm6_core.c"

#define FM_NFACT 128
#define FM_NUSER 32
#define FM_NPATCH (FM_NFACT + FM_NUSER)
#define FM_NHAND 6
#define FM_Q (BLK + CTL)                       /* a FIFO: a chunk's worth and a block */
#define FM_GAIN (1.0f / 16777216.0f)           /* Q24 (one carrier at full: 1.0) -> the mix */

static const uint8_t DX7_FACTORY[FM_NFACT * 128] = {
#include "dx7_factory.inc"
};
static const uint8_t *fm_user;                 /* the user bank (32 packed voices), or none (en_fm_user) */
volatile float en_fm_level;                    /* the FM voice's meter, 0..1 */

static uint32_t fm_max(uint32_t i)             /* (Felucca eng_fm6.c fm6_max) a value's top */
{
    static const uint8_t OPMAX[21] = {99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 3, 3, 7, 3, 7, 99, 1, 31, 99, 14};
    static const uint8_t VMAX[19] = {99, 99, 99, 99, 99, 99, 99, 99, 31, 7, 1, 99, 99, 99, 99, 1, 5, 7, 48};
    if (i < 126u)
        return OPMAX[i % 21u];
    if (i < FP_NAME)
        return VMAX[i - 126u];
    return 126u;
}

/* (Felucca eng_fm6.c fm6_unpack) a 128-byte packed record (the 32-voice bank's) -> the 155-byte voice, every
 * value into its range, a name byte outside 32..126 a space */
static void fm_unpack(const uint8_t *b, uint8_t *v)
{
    uint32_t k, i;
    for (k = 0; k < 6u; k++) {
        const uint8_t *o = b + k * 17u;
        uint8_t *d = v + k * FP_OP;
        for (i = 0; i < 11u; i++)
            d[i] = o[i] & 0x7Fu;
        d[FP_LC] = o[11] & 3u;
        d[FP_RC] = (o[11] >> 2) & 3u;
        d[FP_RS] = o[12] & 7u;
        d[FP_DET] = (o[12] >> 3) & 15u;
        d[FP_AMS] = o[13] & 3u;
        d[FP_KVS] = (o[13] >> 2) & 7u;
        d[FP_OL] = o[14] & 0x7Fu;
        d[FP_MODE] = o[15] & 1u;
        d[FP_FC] = (o[15] >> 1) & 31u;
        d[FP_FF] = o[16] & 0x7Fu;
    }
    for (i = 0; i < 9u; i++)
        v[FP_PR1 + i] = b[102 + i] & 0x7Fu;    /* the pitch envelope, the algorithm */
    v[FP_ALG] &= 31u;
    v[FP_FB] = b[111] & 7u;
    v[FP_OKS] = (b[111] >> 3) & 1u;
    for (i = 0; i < 4u; i++)
        v[FP_LFS + i] = b[112 + i] & 0x7Fu;
    v[FP_LKS] = b[116] & 1u;
    v[FP_LFW] = (b[116] >> 1) & 7u;
    v[FP_LPMS] = (b[116] >> 4) & 7u;
    v[FP_TRNSP] = b[117] & 0x7Fu;
    for (i = 0; i < 10u; i++)
        v[FP_NAME + i] = b[118 + i] & 0x7Fu;
    for (i = 0; i < FP_SIZE; i++) {
        if (i >= FP_NAME)
            v[i] = v[i] < 32u || v[i] > 126u ? ' ' : v[i];
        else if (v[i] > fm_max(i))
            v[i] = (uint8_t)fm_max(i);
    }
}

/* patch i's packed record: 0..127 factory, 128..159 the user bank; none: 0 */
static const uint8_t *fm_record(int i)
{
    if (i >= 0 && i < FM_NFACT)
        return DX7_FACTORY + i * 128;
    if (i >= FM_NFACT && i < FM_NPATCH && fm_user)
        return fm_user + (i - FM_NFACT) * 128;
    return 0;
}

void en_fm_code(int i, char *b)                /* "A01" .. "D32", "U01" .. "U32" */
{
    int k = i < FM_NFACT ? i % 32 : i - FM_NFACT;
    b[0] = i < FM_NFACT ? (char)('A' + i / 32) : 'U';
    b[1] = (char)('0' + (k + 1) / 10);
    b[2] = (char)('0' + (k + 1) % 10);
    b[3] = 0;
}
void en_fm_name(int i, char *b)                /* the voice's own name (10), trailing spaces cut; "EMPTY" */
{
    const uint8_t *r = fm_record(i);
    int k, n = 0;
    if (!r) {
        const char *e = "EMPTY";
        while (*e)
            b[n++] = *e++;
        b[n] = 0;
        return;
    }
    for (k = 0; k < 10; k++) {
        uint8_t c = r[118 + k] & 0x7Fu;
        b[k] = (char)(c < 32u || c > 126u ? ' ' : c);
    }
    for (n = 10; n > 0 && b[n - 1] == ' '; n--)
        ;
    b[n] = 0;
}

/* a received DX7 32-voice bulk dump (F0 43 0n 09 20 00, 4096 bytes, checksum, F7): 1 = good; its 32 voices
 * packed in data (4096) */
int en_fm_bulk_ok(const uint8_t *sx, uint32_t n, const uint8_t **data)
{
    uint32_t i, sum = 0;
    if (n != 4104u || sx[0] != 0xF0u || sx[1] != 0x43u || (sx[2] & 0xF0u) || sx[3] != 0x09u || sx[4] != 0x20u ||
        sx[5] != 0x00u || sx[4103] != 0xF7u)
        return 0;
    for (i = 0; i < 4096u; i++) {
        if (sx[6 + i] & 0x80u)
            return 0;
        sum += sx[6 + i];
    }
    if (((0u - sum) & 0x7Fu) != sx[4102])
        return 0;
    *data = sx + 6;
    return 1;
}

/* ------------------------------------------------------------- a part --- */
/* one patch's notes: the FM voice's (FM_NHAND), or the pad's (EN_NPAD) */
typedef struct {
    uint8_t v[FP_SIZE + 1];                    /* the patch, unpacked */
    int16_t idx;                               /* its number (-1: none yet) */
    fm6_lfo_t lfo;
    int32_t lfo_v, lfo_d, dt[6];
    uint32_t have;                             /* samples in its FIFOs */
} fm_part_t;

typedef struct {
    fm6_note_t n;
    float note, target;                        /* MIDI, fractional: where it sounds, where it glides */
    uint8_t key, vel, down, used;              /* used: sounding (live); down: the key held */
    uint32_t ord;                              /* when it started (the oldest is taken) */
} fm_voice_t;

static fm_part_t fm_hand, fm_padp;
/* (.bss, not the POOL: the SMK-37 POOL keeps the build's 8 KiB headroom) */
static float fm_hq[FM_Q];                      /* the FM voice's FIFO (mono) */
static float fm_pq[EN_NPAD][FM_Q];             /* each pad voice's */
static fm_voice_t fmv[FM_NHAND];
static fm6_note_t fm_padn[EN_NPAD];
static uint8_t fm_pad_was[EN_NPAD], fm_pad_down[EN_NPAD], fm_pad_live[EN_NPAD];   /* (fm_pad_seq, fm_pad_note: amb.c) */
static uint32_t fm_ord;
static uint8_t fm_held[16], fm_nheld;          /* mono: the keys held, the last on top */
static float fm_dc[FM_NHAND + EN_NPAD];

static void fm_part_patch(fm_part_t *pt, int idx)
{
    const uint8_t *r = fm_record(idx);
    uint32_t k;
    if (idx == pt->idx)
        return;
    pt->idx = (int16_t)idx;
    if (r)
        fm_unpack(r, pt->v);
    else {                                     /* an empty slot: a silent patch (every output level 0) */
        for (k = 0; k < sizeof pt->v; k++)
            pt->v[k] = 0;
        for (k = 0; k < 6u; k++)
            pt->v[k * FP_OP + FP_FC] = 1;
        pt->v[FP_TRNSP] = 24;
    }
    fm6_lfo_reset(&pt->lfo, pt->v);
    for (k = 0; k < 6u; k++)
        pt->dt[k] = 0;
}

static int fm_pad_on(void) { return par[P_PADTONE] >= EN_NSOUND; }

static void fm_init(void)                      /* (everything static starts at 0) */
{
    (void)fm6_note_logfreq;                    /* (Felucca's pitch path; here fm_lgf) */
    fm_hand.idx = fm_padp.idx = -1;
    fm_part_patch(&fm_hand, par[P_FM_PATCH]);
    fm_part_patch(&fm_padp, fm_pad_on() ? par[P_PADTONE] - EN_NSOUND : 0);
}

void en_fm_user(const uint8_t *bank)          /* the user bank (32 x 128 bytes), or 0; the patches reread */
{
    fm_user = bank;
    fm_hand.idx = fm_padp.idx = -1;            /* (re-unpacked at the next block) */
}

/* -------------------------------------------------------- the FM voice --- */
static int32_t fm_lgf(float note)             /* MIDI note (fractional) -> msfa's log2 frequency, Q24 */
{
    return FM6_LOGF0 + (int32_t)(note * (16777216.0f / 12.0f));
}

static float fm_key_note(int key)              /* a key's pitch: its octave, the song's, the patch's transpose */
{
    return (float)(key + 12 * (par[P_FM_OCT] + par[P_OCT]) + fm_hand.v[FP_TRNSP] - 24);
}

static int fm_vel(int vel)                     /* the velocity, Vel's share of it (0: always 100) */
{
    int s = par[P_FM_VEL];
    vel = 100 + (vel - 100) * s / 100;
    return vel < 1 ? 1 : vel > 127 ? 127 : vel;
}

static void fm_start(fm_voice_t *f, int key, int vel, int legato)
{
    float n = fm_key_note(key);
    int fresh = !f->used;
    f->key = (uint8_t)key;
    f->vel = (uint8_t)vel;
    f->target = n;
    if (fresh || par[P_FM_GLIDE] == 0)
        f->note = n;
    if (!legato) {
        fm6_note_init(&f->n, fm_hand.v, (int32_t)(n + 0.5f), fm_vel(vel), fresh);
        fm6_lfo_key(&fm_hand.lfo);
    }
    f->used = f->down = 1;
    f->ord = ++fm_ord;
}

static void fm_note_cmd(int key, int vel, int on)
{
    int i;
    if (par[P_FM_MONO]) {                      /* mono: the last key held; legato glides, no new attack */
        fm_voice_t *f = &fmv[0];
        int k;
        for (k = 0; k < fm_nheld; k++)
            if (fm_held[k] == key) {
                for (; k + 1 < fm_nheld; k++)
                    fm_held[k] = fm_held[k + 1];
                fm_nheld--;
                break;
            }
        if (on) {
            int legato = fm_nheld > 0 && f->used && f->down;
            if (fm_nheld == 16) {
                for (k = 0; k < 15; k++)
                    fm_held[k] = fm_held[k + 1];
                fm_nheld = 15;
            }
            fm_held[fm_nheld++] = (uint8_t)key;
            fm_start(f, key, vel, legato);
        } else if (fm_nheld) {
            fm_start(f, fm_held[fm_nheld - 1], f->vel, 1);   /* back to the key still held */
        } else if (f->used && f->down) {
            fm6_note_key(&f->n, fm_hand.v, 0);
            f->down = 0;
        }
    } else if (on) {                           /* poly: the same key again, else a free note, the oldest let go, the oldest */
        fm_voice_t *best = 0;
        for (i = 0; !best && i < FM_NHAND; i++)
            if (fmv[i].used && fmv[i].key == key)
                best = &fmv[i];
        for (i = 0; !best && i < FM_NHAND; i++)
            if (!fmv[i].used)
                best = &fmv[i];
        if (!best) {                            /* none free: the oldest let go, else the oldest */
            fm_voice_t *rel = 0, *old = &fmv[0];
            for (i = 0; i < FM_NHAND; i++) {
                if (!fmv[i].down && (!rel || fmv[i].ord < rel->ord))
                    rel = &fmv[i];
                if (fmv[i].ord < old->ord)
                    old = &fmv[i];
            }
            best = rel ? rel : old;
        }
        fm_start(best, key, vel, 0);
    } else {
        for (i = 0; i < FM_NHAND; i++)
            if (fmv[i].used && fmv[i].down && fmv[i].key == key) {
                fm6_note_key(&fmv[i].n, fm_hand.v, 0);
                fmv[i].down = 0;
            }
    }
    midi(on ? 0x91u : 0x81u, (uint32_t)(key & 0x7F), on ? (uint32_t)vel : 0u);   /* MIDI out: channel 2 */
}

static void fm_quiet(void)
{
    int i;
    for (i = 0; i < FM_NHAND; i++)
        fmv[i].used = fmv[i].down = 0;
    fm_nheld = 0;
}

static float fm_bend(void)                     /* the pitch wheel, in semitones (SMK-37) */
{
#if BOARD_SMK37
    return (float)perf_bend * (1.0f / 4096.0f);
#else
    return 0.0f;
#endif
}

/* one block of the FM voice into its FIFO */
static void fm_hand_block(void)
{
    int32_t bus[CTL], acc[CTL], i, live = 0;
    uint32_t j, alg;
    int32_t fb, modlvl;
    float gl = par[P_FM_GLIDE] ? fm_minf(32.0f / (0.01f * fm_exp2f((float)par[P_FM_GLIDE] * 0.09f) * EN_SR), 1.0f) : 1.0f,
          bend = fm_bend(), pk = 0.0f;
    fm_part_patch(&fm_hand, par[P_FM_PATCH]);
    alg = fm_hand.v[FP_ALG];
    fb = fm_hand.v[FP_FB];
    modlvl = (par[P_FM_BRIGHT] - 50) * (1 << 24) / 40;   /* Bright: the modulators +-1.25 (Q24 log2: 2^24 = 6 dB) */
    fm_hand.lfo_v = fm6_lfo_sample(&fm_hand.lfo);
    fm_hand.lfo_d = fm6_lfo_delay(&fm_hand.lfo);
    for (j = 0; j < CTL; j++)
        acc[j] = 0;
    for (i = 0; i < FM_NHAND; i++) {
        fm_voice_t *f = &fmv[i];
        if (!f->used)
            continue;
        f->note += (f->target - f->note) * gl;
        for (j = 0; j < CTL; j++)
            bus[j] = 0;
        if (fm6_note_compute(&f->n, fm_hand.v, bus, fm_hand.lfo_v, fm_hand.lfo_d, fm_lgf(f->note + bend), alg, fb,
                             fm_hand.dt, modlvl)) {
            for (j = 0; j < CTL; j++) {
                int32_t x = bus[j] - (int32_t)fm_dc[i];
                fm_dc[i] += (float)x * (1.0f / 1024.0f);   /* (~7 Hz high-pass: FM puts sidebands at 0 Hz) */
                acc[j] += x;
            }
            live = 1;
        }
        if (!f->down && fm6_note_done(&f->n, fm_hand.v, alg))
            f->used = 0;
    }
    for (j = 0; j < CTL; j++) {
        float x = (float)acc[j] * FM_GAIN;
        fm_hq[fm_hand.have + j] = x;
        pk = fm_maxf(pk, fm_fabsf(x));
    }
    fm_hand.have += CTL;
    en_fm_level += ((live ? fm_minf(pk * 1.5f, 1.0f) : 0.0f) - en_fm_level) * 0.05f;
}

/* the FM voice into the mix: n samples (render_chunk, before the sidechain) */
static void render_fm(uint32_t n)
{
    uint32_t j;
    const float g = (float)par[P_FM_LVL] * 0.01f * 0.42f, sp = (float)par[P_FM_SPACE] * 0.01f;
    int any = fm_hand.have > 0u, i;
    for (i = 0; i < FM_NHAND; i++)
        any |= fmv[i].used;
    if (!any) {
        en_fm_level *= 0.9f;
        return;
    }
    while (fm_hand.have < n)
        fm_hand_block();
    for (j = 0; j < n; j++) {
        float x = fm_hq[j] * g;
        mixl[j] += x;
        mixr[j] += x;
        esend[j] += x * sp * 0.9f;
        rsend[j] += x * sp * 0.7f;
    }
    fm_hand.have -= n;
    for (j = 0; j < fm_hand.have; j++)
        fm_hq[j] = fm_hq[j + n];
}

/* --------------------------------------------------------------- the pad --- */
/* pad_coefs: voice i sings at this pitch now; a new breath / stab / key bumps fm_pad_seq[i] (amb.c) */
static void fm_pad_block(void)
{
    int32_t bus[CTL];
    uint32_t j, alg;
    int i;
    int32_t fb, modlvl;
    float bend = fm_bend();
    fm_part_patch(&fm_padp, par[P_PADTONE] - EN_NSOUND);
    alg = fm_padp.v[FP_ALG];
    fb = fm_padp.v[FP_FB];
    modlvl = (par[P_BRIGHT] - 35) * (1 << 24) / 40;
    fm_padp.lfo_v = fm6_lfo_sample(&fm_padp.lfo);
    fm_padp.lfo_d = fm6_lfo_delay(&fm_padp.lfo);
    for (i = 0; i < EN_NPAD; i++) {
        pad_t *v = &pad[i];
        fm6_note_t *nt = &fm_padn[i];
        float *q = fm_pq[i] + fm_padp.have;
        int down;
        float note = fm_pad_note[i] + (float)(fm_padp.v[FP_TRNSP] - 24);
        if (fm_pad_seq[i] != fm_pad_was[i]) {  /* a new breath, stab or key: a note-on */
            fm_pad_was[i] = fm_pad_seq[i];
            fm6_note_init(nt, fm_padp.v, (int32_t)(note + 0.5f), v->play ? 100 : (int)(60.0f + 50.0f * v->peak),
                          !fm_pad_live[i]);
            fm6_lfo_key(&fm_padp.lfo);
            fm_pad_live[i] = fm_pad_down[i] = 1;
        }
        if (v->play)                           /* held: the key; else the breath's rise and hang, or a stab's start */
            down = v->play == 1;
        else if (par[P_PAD_MAN])
            down = 0;
        else if (pad_stabs())
            down = stab_env > 0.3f;
        else
            down = v->t < v->fa + v->fh;
        if (fm_pad_down[i] && !down) {
            fm6_note_key(nt, fm_padp.v, 0);
            fm_pad_down[i] = 0;
        }
        for (j = 0; j < CTL; j++)
            q[j] = 0.0f;
        if (!fm_pad_live[i])
            continue;
        for (j = 0; j < CTL; j++)
            bus[j] = 0;
        if (fm6_note_compute(nt, fm_padp.v, bus, fm_padp.lfo_v, fm_padp.lfo_d, fm_lgf(note + bend), alg, fb, fm_padp.dt,
                             modlvl)) {
            float *dc = &fm_dc[FM_NHAND + i];
            for (j = 0; j < CTL; j++) {
                float x = (float)bus[j] - *dc;
                *dc += x * (1.0f / 1024.0f);
                q[j] = x * FM_GAIN;
            }
        }
        if (!fm_pad_down[i] && fm6_note_done(nt, fm_padp.v, alg))
            fm_pad_live[i] = 0;
    }
    fm_padp.have += CTL;
}

static void fm_pad_fill(uint32_t n)            /* render_pad: n samples of each pad voice ready */
{
    while (fm_padp.have < n)
        fm_pad_block();
}
static void fm_pad_used(uint32_t n)
{
    uint32_t i, j;
    fm_padp.have -= n;
    for (i = 0; i < EN_NPAD; i++)
        for (j = 0; j < fm_padp.have; j++)
            fm_pq[i][j] = fm_pq[i][j + n];
}
