/* SPDX-License-Identifier: GPL-3.0-only */
/* HYP-1 on the M-Vave SMK-37: the controls the FM-1 has not. Included by ui.c under BOARD_SMK37 (after the
 * drawing helpers); the FM-1's controls keep their HYP-1 meaning, on the SMK button printed with the same job.
 *
 *   keys F3..G5    as the FM-1's 27: the scale, a voice by hand, the effects, the steps (ui.c)
 *   keys C3..E3,   the scale too (each its note in or out); on the pages of a voice played by hand, that
 *   G#5..C6        voice, below F3 and above G5
 *   16 pads        the steps of the drum lane in the sequencer's pattern, in reading order: the top row
 *                  steps 1-8, the bottom row 9-16. A pad turns its hit on or off, on any page. Lit: the hits
 *                  in the drums' colour, the playhead bright, beats 1 5 9 13 faint
 *   < >            the pads' lane: kick, snare, clap, hat, open hat, perc (as OCT- / OCT+ in the sequencer)
 *   KNOB BANK      the picker on the pads and the screen: the top row pieces 01-08, the bottom row the six
 *                  lanes, then pieces 09 and 10. A piece pad: that piece (SCALE held, playing: on the next
 *                  bar); a lane pad: that lane, and the pads go back to its steps. KNOB BANK again closes
 *                  it; a PRESETS turn shows it for a moment
 *   FADER BANK     what the faders set: A the levels (pad, drone, acid, drums), B the acid (cutoff, reso,
 *                  env mod, decay), C the space and the rest (echo, reverb, noise, drone cutoff). A fader
 *                  takes over once it moves (also after a bank change). The fader card shows the bank
 *   pitch wheel    bend, +-2 semitones, on the pad, the drone and the acid (by hand too)
 *   mod wheel      a filter macro: the acid's and the drone's cutoff open together, up to 3 octaves
 * The wheels never change what is stored. One card at a time: KNOB BANK closes the fader card, FADER BANK
 * the picker. */

#define SMK_NBANK 3
#define SMK_CARD_MS 1800u
static const uint8_t SMK_FADER_PAR[SMK_NBANK][4] = {{P_PAD, P_BASS, P_ACID, P_PULSE},
                                                    {P_AC_CUT, P_AC_RES, P_AC_ENV, P_AC_DEC},
                                                    {P_ECHO, P_REVERB, P_NOISE, P_DR_CUT}};
static const char *const SMK_FADER_LABEL[SMK_NBANK][4] = {{"Pad", "Drone", "Acid", "Drums"},
                                                          {"Cutoff", "Reso", "Env mod", "Decay"},
                                                          {"Echo", "Reverb", "Noise", "Drone cut"}};
static const char *const SMK_BANK_NAME[SMK_NBANK] = {"A LEVELS", "B ACID", "C SPACE"};
/* SMK_LANE3: the lanes in three letters (ui.c) */

static struct {
    uint32_t sbtn;                     /* the four extra buttons held (SB_*) */
    uint8_t fbank;                     /* the fader bank, 0..2 */
    uint8_t flast[4];                  /* each fader's last value, 0xFF: not read yet (takes over once it moves) */
    int8_t ftouch;                     /* the fader moved last (the card shows it), -1 none */
    uint8_t pick;                      /* KNOB BANK: the picker pinned */
    uint32_t pick_until, card_until;   /* the picker after a PRESETS turn; the fader card */
    int32_t pitch, modw;               /* the wheels: pitch -127..127 (0 at rest), mod 0..127 */
    uint8_t okey_voice[SMK_NKEYBED];   /* an outer key playing a voice: its EN_V_* + 1 (the note-off goes there) */
} smk;

static void smk_reset(void)
{
    memset(&smk, 0, sizeof smk);
    memset(smk.flast, 0xFF, sizeof smk.flast);
    smk.ftouch = -1;
}
static int smk_picker(void) { return smk.pick || plat_ms() < smk.pick_until; }
static int smk_fcard(void) { return plat_ms() < smk.card_until; }
static int smk_wheels_off(void) { return (smk.pitch != 0) | (smk.modw != 0) << 1; }
static void smk_preset_turned(void)            /* a PRESETS turn: the picker shows where it is, for a moment */
{
    smk.pick_until = plat_ms() + 2u * SMK_CARD_MS;
    smk.card_until = 0;
}

/* the picker's pads: pieces 01-08 on the top row, the lanes then 09 and 10 on the bottom row */
static int smk_pick_piece(int p) { return p < 8 ? p : p >= 8 + EN_NLANE ? p - EN_NLANE : -1; }
static int smk_pick_lane(int p) { return p >= 8 && p < 8 + EN_NLANE ? p - 8 : -1; }

static void smk_say_lane(void)
{
    char t[16];
    caps(t, EN_LANE_NAME[ui.lane], sizeof t);
    say("PADS:", t);
}

static void smk_input(uint32_t btn)
{
    uint32_t sb = plat_smk_buttons(), sp = sb & ~smk.sbtn, ph = plat_pad_hits(), now = plat_ms(), i;
    uint64_t kh = plat_keybed_hits(), kb = plat_keybed();
    int32_t pitch, modw;
    int shift = (int)(btn >> B_SEL & 1u);
    smk.sbtn = sb;
    if (sp >> SB_KNOB_BANK & 1u) {               /* KNOB BANK: the picker, pinned or put away */
        smk.pick = (uint8_t)!smk_picker();
        smk.pick_until = 0;
        smk.card_until = 0;                      /* (one card at a time) */
    }
    if (sp >> SB_FADER_BANK & 1u) {              /* FADER BANK: the next bank, on the fader card */
        smk.fbank = (uint8_t)((smk.fbank + 1u) % SMK_NBANK);
        memset(smk.flast, 0xFF, sizeof smk.flast);
        smk.ftouch = -1;
        smk.card_until = now + SMK_CARD_MS;
        smk.pick = 0;
        smk.pick_until = 0;
    }
    if (sp & (1u << SB_LEFT | 1u << SB_RIGHT)) { /* < >: the pads' lane */
        ui.lane = (uint8_t)((ui.lane + ((sp >> SB_RIGHT & 1u) ? 1u : EN_NLANE - 1u)) % EN_NLANE);
        ui.touched = -1;
        smk_say_lane();
    }
    for (i = 0; i < SMK_NKEYBED; i++) {          /* the outer keys (the middle 27 are the FM-1's, in ui.c) */
        int vc;
        if (i >= 5u && i < 32u)
            continue;
        if (smk.okey_voice[i] && !(kb >> i & 1u)) {   /* let go: its note off, wherever it went */
            en_note(smk.okey_voice[i] - 1, 48 + (int)i, 0);
            smk.okey_voice[i] = 0;
        }
        if (!(kh >> i & 1u) || shift || ui.fx_menu || ui.view == V_SEQ)
            continue;
        vc = voice_of(ui.view);
        if (by_hand(vc)) {                       /* a voice off the algorithm, its page: the key plays it */
            smk.okey_voice[i] = (uint8_t)(vc + 1);
            en_note(vc, 48 + (int)i, 1);         /* key 0 is C3 (MIDI 48), as F3 is 53 */
        } else {
            toggle_note((int)(i % 12u));
        }
    }
    for (i = 0; i < SMK_NPADS; i++) {
        if (!(ph >> i & 1u))
            continue;
        if (smk_picker()) {
            int pc = smk_pick_piece((int)i), ln = smk_pick_lane((int)i);
            if (pc >= 0 && pc < NPIECES) {
                if (shift && en_playing) {       /* SCALE (SEL) held: the piece on the next bar */
                    char u[12] = "PIECE ";
                    ui.btn_used |= 1u << B_SEL;
                    caps(u + 6, piece_name(pc), 4);
                    if (pc == proj.piece) {
                        ui.piece_q = 0;
                        say(u, "STAYS");
                    } else {
                        ui.piece_q = (uint8_t)(pc + 1);
                        say(u, "NEXT BAR");
                        ui.msg_until = now + 60000u;   /* until it comes */
                    }
                } else if (pc != proj.piece) {
                    ui.piece_q = 0;
                    set_piece(pc);
                }
                if (!smk.pick)
                    smk.pick_until = now + 2u * SMK_CARD_MS;
            } else if (ln >= 0) {                /* a lane: that lane, and its steps on the pads */
                ui.lane = (uint8_t)ln;
                ui.touched = -1;
                smk.pick = 0;
                smk.pick_until = 0;
                smk_say_lane();
            }
        } else {                                 /* a step of the lane: its hit on or off */
            char t[16], u[8];
            int on;
            step_toggle((int)i);
            on = (int)(en_pats[seq_pat()][ui.lane] >> i & 1u);
            caps(t, EN_LANE_NAME[ui.lane], 10);
            {
                int n = 0;
                while (t[n])
                    n++;
                t[n++] = ' ';
                itoa_u(i + 1u, t + n);
            }
            u[0] = 0;
            say(t, on ? "ON" : "OFF");
            (void)u;
        }
    }
    if (sb || kh || ph)
        ui.act_t = now;
    for (i = 0; i < 4u; i++) {                   /* the faders: a parameter each, once a fader moves */
        int v = plat_fader((int)i), k = SMK_FADER_PAR[smk.fbank][i], lo, hi;
        if (v < 0)
            continue;
        if (smk.flast[i] == 0xFFu) {
            smk.flast[i] = (uint8_t)v;
            continue;
        }
        if (v == smk.flast[i])
            continue;
        smk.flast[i] = (uint8_t)v;
        knob_range(k, &lo, &hi);
        knob_set(k, lo + (v * (hi - lo) + 63) / 127);
        smk.ftouch = (int8_t)i;
        if (smk.pick) {                          /* the picker is pinned: a word at the top instead */
            char t[16], u[16];
            knob_text(k, t);
            caps(u, SMK_FADER_LABEL[smk.fbank][i], sizeof u);
            say(u, t);
        } else {
            smk.card_until = now + SMK_CARD_MS;
            smk.pick_until = 0;
        }
        ui.act_t = now;
    }
    if (plat_wheels(&pitch, &modw)) {
        char t[8];
        int32_t st10 = (pitch < 0 ? -pitch : pitch) * 20 / 127;   /* the bend in tenths of a semitone */
        en_perf(pitch * 8192 / 127, modw);      /* the engine: +-2 semitones; the filter macro */
        if (pitch != smk.pitch && st10) {
            t[0] = pitch > 0 ? '+' : '-';
            t[1] = (char)('0' + st10 / 10);
            t[2] = '.';
            t[3] = (char)('0' + st10 % 10);
            t[4] = 0;
            say("BEND", t);
        }
        if (modw != smk.modw && modw) {
            int n;
            itoa_u((uint32_t)(modw * 100 / 127), t);
            for (n = 0; t[n]; n++)
                ;
            t[n++] = '%';
            t[n] = 0;
            say("FILTER", t);
        }
        smk.pitch = pitch;
        smk.modw = modw;
        ui.act_t = now;
    }
}

/* ---------------------------------------------------------------- cards --- */
/* the fader card: the bank's four values as four faders, in the knob strip's columns (the faders sit in a
 * row too); the banks as tabs above */
static void smk_draw_fcard(void)
{
    const uint16_t LAYER[4] = {K_PAD, K_BASS, K_ACID, K_PULSE};
    int i, now_touch = smk.ftouch >= 0;
    for (i = 0; i < SMK_NBANK; i++) {            /* the banks: the one the faders set, lit */
        int32_t x = 8 + i * 77;
        cv_round(x, 2, 70, 17, 4, i == smk.fbank ? K_ACC : K_PANEL);
        text_c(x + 35, 4, &FONT_XS, SMK_BANK_NAME[i], i == smk.fbank ? K_BG : K_DIM);
    }
    for (i = 0; i < 4; i++) {
        int k = SMK_FADER_PAR[smk.fbank][i], lo, hi, v, hot = now_touch && smk.ftouch == i;
        int32_t cx = i * 60 + 30, h;
        uint16_t fill = smk.fbank == 0 ? LAYER[i] : smk.fbank == 1 ? K_ACID : K_PAD;
        char t[16];
        if (i)
            cv_rect(i * 60, 28, 1, 112, K_LINE);
        if (hot)
            cv_round(i * 60 + 3, 25, 54, 118, 6, K_PANEL);
        knob_range(k, &lo, &hi);
        v = knob_get(k);
        caps(t, SMK_FADER_LABEL[smk.fbank][i], sizeof t);
        text_c(cx, 28, &FONT_XS, t, hot ? K_TEXT : K_DIM);
        h = hi > lo ? (int32_t)((v - lo) * 70 / (hi - lo)) : 0;
        cv_round(cx - 4, 48, 8, 74, 3, K_LINE);
        if (h > 0)
            cv_round(cx - 4, 48 + 70 - h, 8, h + 4, 3, hot ? K_ACC : fill);
        cv_round(cx - 10, 48 + 70 - h - 1, 20, 6, 2, hot ? K_ACC : K_TEXT);   /* the fader's cap */
        knob_text(k, t);
        text_c(cx, 126, &FONT_XS, t, hot ? K_ACC : K_TEXT);
    }
}

/* the picker: the pads' two rows as tiles, in the pads' colours; the piece and the lane above */
static void smk_draw_picker(void)
{
    int i;
    int32_t x;
    char t[16];
    x = cv_text(8, 2, &FONT_XS, "PIECE", K_DIM);
    caps(t, piece_name(proj.piece), 4);
    x = cv_text(x + 6, 0, &FONT_B, t, K_TEXT);
    cv_text(x + 8, 2, &FONT_XS, project_saved(proj.piece) ? "STORED" : "FACTORY", K_DIM);
    x = cv_text(132, 2, &FONT_XS, "LANE", K_DIM);
    caps(t, EN_LANE_NAME[ui.lane], sizeof t);
    cv_text(x + 6, 0, &FONT_B, t, K_PULSE);
    for (i = 0; i < 16; i++) {
        int pc = smk_pick_piece(i), ln = smk_pick_lane(i), on;
        int32_t cx = 4 + (i % 8) * 29, cy = i < 8 ? 26 : 74;
        uint16_t bg, fg;
        if (pc >= 0) {
            on = pc == proj.piece;
            bg = on ? K_PAD : ui.piece_q == pc + 1 ? K_ACC : project_saved(pc) ? K_PAD_T : K_PANEL;
            fg = on || ui.piece_q == pc + 1 ? K_BG : K_TEXT;
            itoa_u((uint32_t)pc + 1u, t + 1);
            t[0] = pc + 1 < 10 ? '0' : t[1];
            if (pc + 1 >= 10)
                t[1] = t[2], t[2] = 0;
        } else {
            on = ln == ui.lane;
            bg = on ? K_PULSE : en_pats[seq_pat()][ln] ? K_PULSE_T : K_PANEL;
            fg = on ? K_BG : K_TEXT;
            t[0] = SMK_LANE3[ln][0], t[1] = SMK_LANE3[ln][1], t[2] = SMK_LANE3[ln][2], t[3] = 0;
        }
        cv_round(cx, cy, 27, 40, 5, bg);
        text_c(cx + 13, cy + 14, &FONT_XS, t, fg);
    }
    text_c(120, 124, &FONT_XS, smk.pick ? "PADS PICK  //  KNOB BANK CLOSES" : "PADS PICK", K_DIM);
}

/* 1 = a card has the main area this frame (drawn when it changed) */
static int smk_card(void)
{
    uint32_t h;
    int i;
    if (smk_fcard()) {
        h = hash(hash(0x46414452u, smk.fbank | (uint32_t)(smk.ftouch + 1) << 4), ui.theme);
        for (i = 0; i < 4; i++)
            h = hash(h, (uint32_t)knob_get(SMK_FADER_PAR[smk.fbank][i]));
        if (h != ui.sig[1]) {
            ui.sig[1] = h;
            cv_begin(240, MAIN_H, K_BG);
            smk_draw_fcard();
            cv_blit(0, MAIN_Y);
        }
        return 1;
    }
    if (smk_picker()) {
        h = hash(hash(0x5049434Bu, (uint32_t)proj.piece | (uint32_t)ui.lane << 8 | (uint32_t)smk.pick << 12 |
                                      (uint32_t)ui.piece_q << 16 | (uint32_t)ui.theme << 24),
                 (uint32_t)project_saved(proj.piece));
        for (i = 0; i < EN_NLANE; i++)
            h = hash(h, en_pats[seq_pat()][i] != 0);
        if (h != ui.sig[1]) {
            ui.sig[1] = h;
            cv_begin(240, MAIN_H, K_BG);
            smk_draw_picker();
            cv_blit(0, MAIN_Y);
        }
        return 1;
    }
    return 0;
}

/* ----------------------------------------------------------------- pads --- */
/* the pads' lights in the screen's colours (RGB565 -> the LEDs' 8 bits, scaled: full white is blinding) */
static void smk_pad565(int p, uint16_t c, uint32_t q)   /* q: brightness of 256 */
{
    uint32_t r = ((uint32_t)c >> 11 & 31u) * 255u / 31u, g = ((uint32_t)c >> 5 & 63u) * 255u / 63u,
             b = ((uint32_t)c & 31u) * 255u / 31u;
    plat_pad_rgb(p, (uint8_t)(r * q >> 8), (uint8_t)(g * q >> 8), (uint8_t)(b * q >> 8));
}

static void smk_leds(void)
{
    int p;
    plat_smk_leds((uint32_t)smk.pick << SB_KNOB_BANK | (uint32_t)(smk.fbank != 0) << SB_FADER_BANK);
    if (smk_picker()) {                          /* the pieces (the one playing bright) and the lanes */
        for (p = 0; p < SMK_NPADS; p++) {
            int pc = smk_pick_piece(p), ln = smk_pick_lane(p);
            if (pc >= 0 && pc < NPIECES) {
                if (ui.piece_q == pc + 1)        /* on its way: blinking */
                    smk_pad565(p, K_ACC, (ui.frame / 8u) & 1u ? 180u : 30u);
                else
                    smk_pad565(p, K_PAD, pc == proj.piece ? 200u : project_saved(pc) ? 50u : 14u);
            } else if (ln >= 0) {
                smk_pad565(p, K_PULSE, ln == ui.lane ? 200u : en_pats[seq_pat()][ln] ? 40u : 12u);
            } else {
                plat_pad_rgb(p, 0, 0, 0);
            }
        }
        return;
    }
    {   /* the steps of the lane: hits in the drums' colour, the playhead bright, beats 1 5 9 13 faint */
        uint16_t bits = en_pats[seq_pat()][ui.lane];
        int pos = en_playing && en_pat_now == seq_pat() ? en_drum_pos : -1;
        for (p = 0; p < SMK_NPADS; p++) {
            int on = (int)(bits >> p & 1u);
            if (p == pos)
                smk_pad565(p, on ? K_ACC : K_TEXT, on ? 230u : 60u);
            else if (on)
                smk_pad565(p, K_PULSE, en_pulse_rest ? 24u : 70u);
            else if ((p & 3) == 0)
                smk_pad565(p, K_TEXT, 8u);
            else
                plat_pad_rgb(p, 0, 0, 0);
        }
    }
}
