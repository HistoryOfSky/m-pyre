# Changelog

## M-PYRE 1.0.1

- **No more lost notes under load:** when the FM-1 runs short of time it lets voices go, as before, but never one just
  struck (the last ~0.15 s): a chord's notes and a key pressed always sound; ringing tails go first.
- **Guitars and basses ring on in four notes at most:** the next one damps the oldest, as a hand on the strings does
  (fewer voices at once, less load).
- **Recording in time:** a key's press is timed when it happens, and the recording places the hit where it was played,
  also when the FM-1 is busy (fast kicks no longer slip a step).
- **The installer writes the sound pack** right after the firmware, when the FM-1 does not have it yet (it asks before
  replacing other samples in USR1 to USR4).
- The installer page links History Of Sky's songs on Spotify and Instagram.

## M-PYRE 1.0

The first release of M-PYRE, a riffbox firmware for rock and metal on the M-VAVE FM-1. It is based on
[SLOOP](https://github.com/isod89/sloop-fm1) 2.8.1 by isod89, which is based on
[Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita / Hügelton Instruments. Everything SLOOP 2.8.1 has
that is not listed below works as it does there (see `reference/sloop/` for SLOOP's own documentation).

### Sounds

- **Real guitars and bass.** History Of Sky's own DI guitar (11 notes, A1 to A5), palm mutes (8 notes) and a picked
  bass (8 notes, B0 to C4). They are tuned and looped, and stored in the sound pack: the editor writes it into the
  four user sample slots.
- **Guitar presets:** CLEAN GTR, CRUNCH GTR, HI-GAIN, DJENT GTR, LEAD GTR, PALM MUTE, DI GUITAR. The rhythm guitars
  sit an octave down and the lead at pitch.
- **Bass presets:** PICK BASS, DRIVE BASS, HEAVY BASS, an octave below the guitars.
- **Synths for rock:** MOOG FUZZ, MOOG BASS, MOOG LEAD, LADDER PAD and the rest of the HYBRID voice (from Clipper by
  3dSam), next to ANALOG, DIGITAL, PHASE, LOFI, TRIO, DRAWBAR and FM6. FORMANT, GRAIN, PHYS and NOISE were removed to
  make room.
- **HEAVY KIT:** a fully editable kit based on realistic drum samples (kick, snare, ghost, hi-hats, ride and bell, three toms,
  two crashes, china), with four shadings (DEEP, TIGHT, BRIGHT, DUST). The synthesised kits 808, 909, 606, 80S,
  VINTAGE, TRAP and INDUSTR stay.
- **Drum lanes for rock:** PEDAL hi-hat, RIDE and BELL, three toms, CRASH, CRASH 2, CHINA, REV CRASH, DROP.

### Amps and effects

- **Five amps:** CLEAN, CRNCH, HIGH, LEAD and BASS, each drawn on the screen as its own amp. GAIN, BASS, MID and
  TREB on the panel; AMP, CAB, PRES and COMP above it.
- **HIGH and LEAD voiced on the band's own guitar:** fitted to History Of Sky's DI and the same DI through their amp
  and cabinet: a tighter low end, mids that cut through, the highs as recorded, and the cabinet at the full sample
  rate.
- **Cabinets:** the band's own impulse responses for clean, crunch, high-gain, lead and bass.
- **Doubled rhythm guitars:** with the CLEAN, CRNCH and HIGH amps the guitar plays left and right, the right side a
  second take a little later. **GTR DOUBLE** (menu > SEQ) switches it off.
- **The FX as a pedalboard.** The first FX page is the overview of the four mixes (DST, MOD, DLY, REV). Then come
  four pedal pages, each drawn as its pedal:
  - PEDAL: BOOST, OD, DIST and FUZZ (DRIVE, TONE, LEVEL), and OCT (OCT+, OCT-, LEVEL)
  - MOD: PHASR, FLANG, CHOR and TREM (RATE, DEPTH, MIX)
  - DELAY: TIME, FDBK, TONE, MIX
  - REVERB: TYPE (ROOM, HALL, PLATE, SPRIN, CLOUD), SIZE, TONE, MIX
- **Mixes that mix:** at 50 % the effect is as loud as the dry sound, at 100 % only the effect is heard.
- **Delay and reverb start OFF:** the leftmost knob of each (TIME, TYPE) switches it on.
- **Wah, whammy, volume pedal and compressor** on every guitar and bass track.
- **Both cores shared by load:** the tracks go to the FM-1's two cores so that each has about as much to do (SLOOP
  always gave the second core the third track and the drums); the full-rate cabinets cost a quarter of before.

### Playing

- **The LIVE view** (tap HOME): three groups of four controls, tap HOME again for the next.
  - SOUND (synths only): CUT, RES, ATK, REL
  - PLAY: CHORD, BEND, VIB, PALM
  - PLAY 2: LEG, WHAM, WAH, PVOL
- **Chords for guitar:** PW (power chord), PW+, OCT and TERZ.
- **BEND in steps:** a turn right bends a semitone, a strong turn a whole tone, any turn left lets go.
- **Arpeggios on guitar and bass** ring to the next note (the ARP's GATE at 100 % in their presets).
- **CHORD NAME** (menu > SCREEN, off by default): the LIVE view names the chord or note being played, in large type
  over the scope.
- **Palm mutes, slides and accents per step:** the note sheet's ART row (NORM, PALM, SLIDE, HAMMR), and FORCE goes on
  past ACC to PALM.
- **Drum sound editor:** EDIT on the drum track shows the kit. Each sound has LEVEL, PUNCH (a softer or harder
  attack), PITCH and DECAY, and they are kept with the project. The drums' LIVE view draws the whole kit as it plays,
  with LEVEL, PUNCH, LO CUT and HI CUT for the kit.
- **HOME:** tap for the LIVE view, hold for the TRACKS screen. The menu moved to GLO held for about 1.2 seconds.

### Look

- **New screen design:** the Barlow Condensed font, a header with the tempo and the page's place, a 2 x 2 layout of
  the values, ember gauges and slanted track tiles.
- **Palettes:** EMBER (the default, nearly black with fire colours), AMBER, RED, MONO and LIGHT.
- **Boot logo:** History Of Sky's artwork, shown for about three seconds.
- **The amp, pedal and kit views** are drawn on the screen.

### Tools

- **Installer, editor and web app** renamed to M-PYRE. The editor installs the sound pack (Sound pack > Install
  sound pack).
- **The editor in M-PYRE's look** (the EMBER colours and Barlow Condensed, like the installer), and with M-PYRE's pages:
  the Sound page shows AMP and AMP 2, PLAY and PLAY 2 (the LIVE view's controls), and the pedalboard, as on the FM-1.
- **The video:** the installer page plays M-PYRE's promo video (`assets/mpyre/video/`).
- **USB name:** the FM-1 shows up as "M-PYRE" by "History Of Sky" on a computer. The installer and the editor still
  find it under its old names.
- **Browser emulator** (`tools/build_emu.py`, `web/emu.html`): the firmware running in a web page, for trying songs
  and sounds without the FM-1.
- **Eight demo songs**, rock to doom (`tools/demo_songs.py`), as projects and renders.

### Removed

- The full-screen visualiser, SLOOP's DIST effect, the INSERT 2 and CHORUS pages, the FILTER and SLICER pages on
  sample tracks, the HORNS, FLUTE and SCRCH sample sets.

### Projects

- M-PYRE saves projects in its own format (`MPY1`). Projects saved by SLOOP 2.6 to 2.8 are not opened; projects of
  older SLOOP versions are converted when loaded. Save a backup with the editor before you install.
