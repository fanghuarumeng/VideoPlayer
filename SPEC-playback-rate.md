# Spec: playback-rate

## Objective

Client playback supports discrete speed control: **0.5x / 1x / 2x / 3x**.

**Success:** user can change speed from the player chrome; video timing follows the selected rate; 1x restores normal audio.

## Behavior

- Speeds: `0.5`, `1.0`, `2.0`, `3.0` only (snap nearest if given other values).
- UI: rate button on the control bar (next to download) opens a small menu.
- Rate persists across pause; applying a new rate mid-play re-anchors the clock without seek.
- New play keeps the last selected rate.
- **Non-1x: audio is muted** (avoids QAudioOutput overrun / chipmunk without a tempo filter). Status line notes this.

## Boundaries

- **Always:** change rate via `FfmpegPlaybackEngine::PaceToPts` wall-clock scaling.
- **Ask first:** pitch-preserving tempo (rubberband / atempo filter chain).
- **Never:** continuous arbitrary rates outside the four discrete values.

## Success Criteria

- [ ] Menu offers 0.5x / 1x / 2x / 3x and updates the button label.
- [ ] 2x/3x finish a segment roughly 2×/3× faster; 0.5x roughly twice as long.
- [ ] 1x plays with sound; other rates are silent.
