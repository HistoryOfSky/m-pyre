# M-PYRE manual

M-PYRE 1.0, the riffbox firmware for the M-VAVE FM-1, by History Of Sky.

This manual covers what M-PYRE adds to the FM-1: the guitar and bass sounds, the amps, the pedals, the LIVE view and
the drums. The rest — recording, the step sequencer, song mode, MIDI, USB audio and the menu — works as in SLOOP 2.8.1,
which M-PYRE is built on. SLOOP's [guide](reference/sloop/GUIDE.md) describes it in full.

## Contents

1. [The tracks](#1-the-tracks)
2. [The controls](#2-the-controls)
3. [The screen](#3-the-screen)
4. [Sounds](#4-sounds)
5. [The amp](#5-the-amp)
6. [The effects](#6-the-effects)
7. [The LIVE view](#7-the-live-view)
8. [Writing riffs](#8-writing-riffs)
9. [The drums](#9-the-drums)
10. [The sound pack](#10-the-sound-pack)
11. [Settings](#11-settings)

## 1. The tracks

M-PYRE has four tracks. **ALGORITHM** selects one.

| Track | Starts as | Colour |
| --- | --- | --- |
| 1 | HI-GAIN: the rhythm guitar | red |
| 2 | LEAD GTR: the lead guitar | brown |
| 3 | DRIVE BASS: the bass | olive |
| 4 | HEAVY KIT: the drums | grey |

Tracks 1 to 3 can play any sound: a guitar, a bass, a synth, keys. Track 4 is always the drums.

## 2. The controls

| Control | What it does |
| --- | --- |
| **HOME** | tap: the LIVE view; tap again: its next group · hold: the TRACKS screen |
| **GLO** held (about 1.2 s) | the menu (settings, ABOUT); HOME held closes it |
| **ALGORITHM** | the selected track: 1, 2, 3, 4 (drums) |
| **PRESETS** | the selected track's sound, or the drum kit |
| **KNOB 1–4** | the four values at the top of the screen |
| **SELECT** | on a page: the previous / next page of its group · on HOME: the tempo |
| **FX** | tap: the effects (overview, PEDAL, MOD, DELAY, REVERB) · hold: punch-in effects |
| **EDIT** | tap on a guitar or bass track: the amp · on the drum track: the drum sound editor · hold: a layer |
| **SEQ** | tap: the NOTES page (synth tracks) or the drum grid · on the drums: grid ↔ kit |
| **PLAY · REC · SAVE · ARP · ENV · LFO · SEL** | as in SLOOP |
| **OCT− / OCT+** | octave (both: back to 0) · on the drum track, held: ghost / hard hits |
| **EDIT + OCT− / OCT+** | undo / redo |

## 3. The screen

Most pages share one layout. The header shows the tempo, the loop and its steps, the page's name and its place in its
group (for example *FX 2/5*). Under it are the four values the knobs turn, each with a gauge. On the pedal and amp
pages the device is drawn below, its knobs moving with yours.

The footer shows the track's engine and sound.

## 4. Sounds

PRESETS lists the sounds by group:

| Group | Sounds |
| --- | --- |
| GTR | CLEAN GTR, CRUNCH GTR, HI-GAIN, DJENT GTR, LEAD GTR, PALM MUTE, DI GUITAR |
| BASS | PICK BASS, DRIVE BASS, HEAVY BASS, and synth basses (MONO, SUB, FAT, REESE, SAW, ACID 303, FM BASS …) |
| KEYS | grand piano, Rhodes, Wurli, Clav, organs (ROCK ORGAN, DIRTY B3), MELLOTRON |
| STR | strings and pads |
| SYN | leads and brass, MOOG FUZZ |
| HYBR | the HYBRID voice: MOOG BASS, MOOG LEAD, LADDER PAD and more |
| MALL, RETR | mallets and bells, chiptune |

The guitar and bass sounds play History Of Sky's own recordings. They need the sound pack (see
[The sound pack](#10-the-sound-pack)); without it they are silent.

The rhythm guitars play an octave down, the lead guitar at pitch, the basses an octave below the guitars: a riff
played on the same keys on all three lines up.

## 5. The amp

On a guitar or bass track with an amp on, **EDIT** opens the amp. **SELECT** switches between its two pages.

**AMP** (the top row):

| Knob | |
| --- | --- |
| AMP | OFF, CLEAN, CRNCH, HIGH, LEAD, BASS |
| CAB | the cabinet: AUTO (the amp's own), CLEAN, CRNCH, HIGH, LEAD, BASS, OFF |
| PRES | presence: the top end after the amp |
| COMP | a compressor before the amp (OFF to 100 %) |

**AMP 2** (the amp's panel): **GAIN**, **BASS**, **MID**, **TREB**.

Each amp is drawn as its own: CLEAN a tweed combo, CRNCH black and gold, HIGH diamond plate, LEAD and BASS their own
heads. The cabinets are History Of Sky's impulse responses.

With **CLEAN**, **CRNCH** and **HIGH** the guitar is doubled: the left channel plays as it is, the right a second take
a few milliseconds later, its timing drifting slightly. LEAD and BASS stay in the middle.

**The BASS amp:** GAIN goes from clean to fuzz. TREB below the middle closes the tone down to almost only the low
end; above the middle it brings out the clank of the strings.

## 6. The effects

Tap **FX**. **SELECT** walks through the five pages.

### FX 1/5: the overview

The mix of each effect on the selected track:

| Knob | |
| --- | --- |
| DST | the drive pedal's mix |
| MOD | the modulation's mix |
| DLY | the delay's mix |
| REV | the reverb's mix |

Every MIX works the same way: at 0 % only the dry sound, at 50 % the effect as loud as the dry sound, at 100 % only
the effect.

### FX 2/5: PEDAL

**PEDAL** picks the pedal: OFF, BOOST, OD, DIST, FUZZ, OCT.

- BOOST and OD: **DRIVE**, **TONE**, **LEVEL**. DIST: **DIST**, **TONE**, **LEVEL**. FUZZ: **FUZZ**, **TONE**,
  **LEVEL**.
- OCT, an octaver: **OCT+** (the octave up), **OCT−** (the octave down), **LEVEL** (the original note).

### FX 3/5: MOD

**MOD** picks the effect: OFF, PHASR, FLANG, CHOR, TREM. Then **RATE**, **DEPTH**, **MIX**.

### FX 4/5: DELAY

**TIME**, **FDBK** (feedback), **TONE**, **MIX**. TIME is in note values: OFF, 1/4, 1/8, 1/16, 1/32, 8T, 16T, 1/8D,
1/16D, 1/4D. The delay is shared by all tracks; MIX is the selected track's.

TIME starts at **OFF**: turn it to a note value to switch the delay on.

### FX 5/5: REVERB

**TYPE** (OFF, ROOM, HALL, PLATE, SPRIN, CLOUD), **SIZE**, **TONE**, **MIX**. Like the delay it is shared, MIX is
the selected track's, and TYPE starts at **OFF**.

### Punch-in effects

Hold **FX** and press a key: a punch-in effect for as long as you hold it. They work as in SLOOP. FREEZE and the
THROWs bring the reverb and delay in even when they are OFF.

## 7. The LIVE view

Tap **HOME**: the four knobs become the things you play with. Tap **HOME** again for the next group.

| Group | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| SOUND (synths only) | CUT | RES | ATK | REL |
| PLAY | CHORD | BEND | VIB | PALM |
| PLAY 2 | LEGATO | WHAMMY | WAH | VOL |

- **CHORD:** each key plays a chord. PW is a power chord (root and fifth), PW+ adds the octave, OCT plays the octave,
  TERZ the third; TRIAD, 7TH, 9TH and SUS4 as in SLOOP.
- **BEND:** bends the notes up, as a string bend: a quarter, a half or a whole tone.
- **VIB:** vibrato, faster as you turn it up.
- **PALM:** ON plays palm mutes instead of open notes.
- **LEGATO:** notes played while one is held glide or hammer into each other instead of starting again.
- **WHAMMY:** the whammy bar, down and up from the middle.
- **WAH:** a wah pedal; OFF at the left.
- **VOL:** a volume pedal, for swells and fades.

The scope between the header and the footer shows the track's sound. With **CHORD NAME** on (menu > SCREEN, off by
default) the scope is dimmed and the chord the track plays is named over it in large type: a power chord (E5), a triad
(E, E MIN), 7, MAJ7, MIN7, SUS2, SUS4, DIM, AUG, ADD9 - or, for a single note, the note with its octave (A2). The notes
are listed under it. When you let go, the last name stays, dimmed. On the drum track the LIVE view shows the kit
(see [The drums](#9-the-drums)).

## 8. Writing riffs

Record with **REC** or write notes on the **NOTES** page (tap **SEQ**), as in SLOOP. M-PYRE adds:

- **FORCE PALM:** on the NOTES page, KNOB 4 (FORCE) goes GHOST, SOFT, NORM, HARD, ACC, then **PALM**: the note is
  palm-muted.
- **The note's ART:** hold **OCT+** on a note for its sheet. Its first row, **ART**, sets how the note is played:
  NORM, **PALM** (palm mute), **SLIDE** (glides into the next note), **HAMMR** (a hammer-on or pull-off into the next
  note, no new pick attack).
- **A chord on one step:** with CHORD set, a single note on a step plays its chord.

## 9. The drums

The drum kits: **HEAVY KIT** (recorded by History Of Sky) and its shadings **DEEP**, **TIGHT**, **BRIGHT** and
**DUST**; the synthesised kits **808**, **909**, **606**, **80S**, **VINTAGE**, **TRAP**, **INDUSTR**; your own kits
(USR, SYN) as in SLOOP.

The 16 drum sounds: KICK, SNARE, GHOST, HAT, OPEN HAT, PEDAL, RIDE, BELL, HI TOM, MID TOM, FLOOR, CRASH, CRASH 2,
CHINA, REV CRASH, DROP.

### The drum sound editor

On the drum track, **EDIT** opens the sound editor: the 16 sounds with their levels, the selected one drawn large.
**SELECT** picks a sound. The knobs:

| Knob | |
| --- | --- |
| LEVEL | the sound's level |
| PUNCH | the attack: to the left softer, to the right harder and more driven (not louder) |
| PITCH | the tuning |
| DECAY | shorter to the left, a longer tail to the right, in small steps |

The settings are kept with the project. **SEQ** switches between the drum grid and the kit.

### The drums' LIVE view

Tap **HOME** on the drum track: the whole kit, each drum lighting up as it plays. The knobs shape the whole kit:
**LEVEL**, **PUNCH**, **LO CUT**, **HI CUT**.

## 10. The sound pack

The guitar and bass recordings are too large for the firmware. They live in the FM-1's four user sample slots:

| Slot | |
| --- | --- |
| USR1, USR2 | the guitar, A1 to A5 |
| USR3 | the palm mutes |
| USR4 | the bass, B0 to C4 |

To install them: open the editor, **Connect**, go to **samples**, press **Install sound pack** and choose
`soundpack.bin`. Once installed they stay on the FM-1, also through firmware updates. Loading your own samples into
these slots replaces them; install the sound pack again to get them back.

## 11. Settings

Hold **GLO** for about 1.2 seconds for the menu. M-PYRE adds the palettes **EMBER** (the default), **AMBER**,
**RED**, **MONO** and **LIGHT**, and **CHORD NAME** (SCREEN, off by default; see [The LIVE view](#7-the-live-view)). **ABOUT** shows the version and the credits.

The other settings are SLOOP's: see its [guide](reference/sloop/GUIDE.md).
