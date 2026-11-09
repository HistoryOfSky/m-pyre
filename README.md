<p align="center"><img src="assets/mpyre/splash.png" alt="M-PYRE by History Of Sky" width="240"></p>

<p align="center"><b>It's not a groovebox. It's a RIFFBOX.</b><br>
A firmware for the M-VAVE FM-1, for rock and metal — from idea to song.<br>
Made by History Of Sky. Free and open source (GPL-3.0), based on <a href="https://github.com/isod89/sloop-fm1">SLOOP</a>
and <a href="https://github.com/hugelton/Felucca">Felucca</a>.</p>

<p align="center">
<a href="https://historyofsky.github.io/m-pyre/"><b>Install from the browser</b></a> ·
<a href="MANUAL.md">Manual</a> ·
<a href="CHANGELOG.md">What's new</a> ·
<a href="../../releases">Releases</a> ·
<a href="../../issues">Report a bug</a>
</p>

<p align="center"><a href="https://historyofsky.github.io/m-pyre/media/m-pyre-promo.mp4"><img
src="assets/mpyre/video/m-pyre-promo-poster.jpg" width="640" alt="The M-PYRE video: the FM-1's screens playing History Of Sky's The Quintessence"></a><br>
<a href="https://historyofsky.github.io/m-pyre/media/m-pyre-promo.mp4"><b>Watch the video</b></a> (2 minutes, everything in it rendered by the firmware)</p>

<p align="center"><b>Music by History Of Sky</b><br>
<a href="https://open.spotify.com/track/0zcH8TBsfImHzdnM2COh83">The Prism</a> ·
<a href="https://open.spotify.com/track/41KA0kkv4ogBhnwRf7uqUN">The Quintessence</a> ·
<a href="https://open.spotify.com/track/2sZQYQhxmWdmGsvXrxyEB4">The Truth (Parts I–III)</a> ·
The Fountain (coming soon)<br>
<a href="https://open.spotify.com/artist/4ehdjRy2ETpUSkIbtuEG3S">Spotify</a> ·
<a href="https://www.instagram.com/historyofsky/">Instagram</a></p>

---

M-PYRE turns the FM-1 into a box for writing riffs, from the first idea to the song. Record the drums, the bass, the
rhythm guitars and the lead on its four tracks: real guitar and bass samples through amp models and the band's own
cabinet IRs, a fully editable kit based on realistic drum samples. The effects are a pedalboard on the screen. The
step sequencer knows palm mutes, slides, accents and power chords, and the LIVE view puts live controls for realistic
guitar performances on the knobs: bend, vibrato, palm mute, whammy, wah. Write a riff, loop it, play over it.

> **Status:** 1.0. Custom firmware: install it at your own risk. Your FM-1 can go back to the official firmware at
> any time (see [Going back](#going-back)).

## Screenshots

<p>
<img src="assets/screens/mpyre/tracks.png" width="240" alt="TRACKS: the four tracks and their steps">
<img src="assets/screens/mpyre/amp.png" width="240" alt="The amp view: the HIGH amp">
<img src="assets/screens/mpyre/pedal.png" width="240" alt="The PEDAL page: an overdrive">
<img src="assets/screens/mpyre/live.png" width="240" alt="The LIVE view: chords, bend, vibrato, palm mute">
<img src="assets/screens/mpyre/mod.png" width="240" alt="The MOD page: a phaser">
<img src="assets/screens/mpyre/kit.png" width="240" alt="The drums' LIVE view: the kit as it plays">
</p>

## Features

- **Guitar and bass, sampled:** History Of Sky's DI guitar, palm mutes and picked bass, tuned and looped. Presets:
  CLEAN GTR, CRUNCH GTR, HI-GAIN, DJENT GTR, LEAD GTR, PALM MUTE, DI GUITAR, PICK BASS, DRIVE BASS, HEAVY BASS.
- **From idea to song:** four tracks to record on — drums, bass, rhythm guitar, lead — with the step sequencer, song
  mode and sections A–D to turn a riff into a song.
- **Five amps** drawn on the screen (CLEAN, CRNCH, HIGH, LEAD, BASS) with GAIN, BASS, MID, TREB, presence and a
  compressor, through the band's own cabinet IRs. HIGH and LEAD are voiced on the band's own guitar and amp. Rhythm
  guitars are doubled left and right.
- **A pedalboard:** BOOST, OD, DIST, FUZZ and an octave pedal; PHASER, FLANGER, CHORUS, TREMOLO; a delay; a reverb
  with ROOM, HALL, PLATE, SPRING and CLOUD. Wah, whammy and a volume pedal on every guitar and bass track.
- **HEAVY KIT:** a fully editable kit based on realistic drum samples, with a hi-hat pedal, ride bell, three toms,
  two crashes and a china, plus the synthesised kits 808, 909, 606, 80S, VINTAGE, TRAP and INDUSTR. Each sound has its
  own level, punch, pitch and decay, and the kit is drawn on the screen as it plays.
- **Live controls for realistic guitar performances:** power chords (PW, PW+, OCT, TERZ), bend, vibrato, palm mutes,
  legato, whammy and wah on the LIVE view; palm mutes, slides and hammer-ons per step in the sequencer. CHORD NAME
  (in the menu) shows the chord you play.
- **8 synth engines and sampled instruments:** ANALOG, DIGITAL, PHASE, LOFI, TRIO, DRAWBAR, FM6 and HYBRID (MOOG FUZZ,
  MOOG BASS, MOOG LEAD …), organs, pianos, Mellotron, strings and pads.
- **Everything SLOOP does:** the step sequencer with parameter locks and micro timing, song mode with sections A–D,
  punch-in effects, MIDI in and out, MIDI clock, USB audio, the web editor.

## Install

1. Open **[the M-PYRE installer](https://historyofsky.github.io/m-pyre/)** in **Chrome or Edge** on a computer.
2. Connect the FM-1 by USB: a **data** cable, directly (no hub).
3. Press **INSTALL M-PYRE**, allow MIDI access and wait for *Done*. The FM-1 restarts on the M-PYRE logo, and the
   installer writes the sound pack (the guitars, palm mutes and bass) into its four sample slots right after — about a
   minute, only when they are not there yet. The drums are in the firmware.

   (By hand, in the editor: **Connect** → **samples** → **Install sound pack** with `soundpack.bin` from the
   [release](../../releases).)

**Before you install:** M-PYRE does not open projects saved by SLOOP 2.6 to 2.8, and the sound pack uses all four
user sample slots (USR1 to USR4). If you have music or samples on the FM-1 you want to keep, save a backup with your
current editor first.

### Other ways

- **Python:** `python3 tools/fm1_install.py m-pyre-1.0.fwsc` (needs `pip install mido python-rtmidi`).
- **Build it yourself:** see [BUILDING.md](BUILDING.md).

### Going back

To return to the official firmware, use M-VAVE's own updater, **M-UPGRADE** (m-vave.com → Downloads → PC Software),
with the official firmware (PC Firmware → FM-1 V15). If it does not find the FM-1, close every other app and browser
tab that uses MIDI.

### Rescue

- **The FM-1 no longer starts M-PYRE:** hold **OCT−** alone while switching it on (*M-PYRE USB RESCUE*), then install
  again.
- **An install was cut off:** the FM-1 stays in update mode; press INSTALL again and it finishes.
- If an FM-1 no longer starts at all, recovery needs
  [FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter).

## Your first riff

1. Switch the FM-1 on. Track 1 is a rhythm guitar (HI-GAIN), track 2 the lead, track 3 the bass, track 4 the drums.
2. Press **ALGORITHM** to pick a track and **PRESETS** to change its sound.
3. On track 4, press **REC** and play a beat on the keys. Press **REC** again on the "1" after your last bar: the
   loop closes, its length sets the tempo, and it plays.
4. Go to track 1, press **REC** and play a riff over it.
5. Tap **SEQ** for the NOTES page: **KNOB 1** picks a note, **KNOB 4** (FORCE) set to **PALM** palm-mutes it.
6. Tap **HOME** for the LIVE view and play over the loop: chords, bend, vibrato and palm mutes are on the four knobs.

The [manual](MANUAL.md) explains the amp view, the pedals, the LIVE view and the drum editor.

## Compatibility

| | |
| --- | --- |
| Device | M-VAVE FM-1 (the official firmware can be put back at any time) |
| Installer and editor | **Chrome or Edge** on Windows, macOS or Linux (Web MIDI with SysEx) |
| Cable | a USB **data** cable, plugged directly (no hub) |
| USB audio | any computer that takes a class-compliant USB audio input (no driver) |
| On a computer | the FM-1 shows up as **M-PYRE** (MIDI and audio) |

## Documentation

- [MANUAL.md](MANUAL.md): how to use M-PYRE
- [CHANGELOG.md](CHANGELOG.md): what changed from SLOOP 2.8.1
- [BUILDING.md](BUILDING.md): build the firmware, run the tests
- [LICENSING.md](LICENSING.md): licences and credits
- [reference/sloop/](reference/sloop/): SLOOP's own manual and guide. The sequencer, song mode, MIDI, USB audio and
  the menu work as described there.

## Credits

- **M-PYRE:** History Of Sky (samples, cabinet IRs, artwork, sound design, the video and its music), with Claude
  (Anthropic) on the code.
- **[SLOOP](https://github.com/isod89/sloop-fm1)** by **isod89**: the groovebox M-PYRE is built on, with its
  contributors (see [reference/sloop/README-SLOOP.md](reference/sloop/README-SLOOP.md#credits-and-thanks)).
- **[Felucca](https://github.com/hugelton/Felucca)** by **Leo Kuroshita** (@kurogedelic) / **Hügelton Instruments**:
  the engines, the sequencer, the editor and the installer SLOOP is built on.
- **Clipper** by **3dSam**: the HYBRID voice (GPL-3.0).
- Piano, vibes and strings: Versilian Studios VSCO-2 CE and VCSL (CC0). Fonts: Barlow Condensed on the FM-1,
  Terminus in the editor, and Black Ops One, Damion, Pacifico, Racing Sans One and Russo One for the amp names (all
  SIL OFL 1.1). Icons: Fukiai (MIT, Hügelton Instruments). PHASE after CrispyZebra (GPL); FM6: msfa from Dexed
  (Apache-2.0).

## Licence

Code: GPL-3.0-only (see [LICENSE](LICENSE)). History Of Sky's guitar and bass samples, cabinet IRs and artwork: CC BY 4.0; the
drum samples: CC0 (see [LICENSING.md](LICENSING.md)). No
warranty. M-VAVE and FM-1 are trademarks of their owners; M-PYRE is not affiliated with M-VAVE.
