# Sprite Generation Prompt System

## Goal

Generate a consistent pixel art character sprite pipeline that starts from one locked seed frame and expands into animation strips, sliced frames, and a final atlas.

The seed frame is the source of truth for every later output.

---

## Core Rules

1. Never proceed to animation until the seed frame is approved.
2. Always reuse the exact same style language in every prompt.
3. Always attach the seed frame for every image-to-image animation request.
4. Keep frame dimensions identical across all animations.
5. If the character drifts in silhouette, palette, or proportions, regenerate from seed instead of patching individual frames.
6. Transparent background is required for every output.
7. Pixel art only. No anti-aliasing, no painterly rendering, no blur, no texture smoothing.

---

## Recommended Defaults

Use these defaults unless a project explicitly needs something else.

- Frame size: `64x64`
- Facing direction: `right`
- Background: `transparent`
- Art style: `clean pixel art, no anti-aliasing, flat colors`
- Centering: `centered in frame`
- Output readiness: `game-ready asset`

---

## Phase 1: Seed Frame

### Prompt 1A: Initial Character Concept

```text
Pixel art game sprite, [CHARACTER DESCRIPTION], idle standing pose,
facing right, transparent background, 64x64 pixels, clean pixel art style,
no anti-aliasing, flat colors, [PALETTE DESCRIPTION],
single frame, centered in frame, game-ready asset
```

### Example

```text
Pixel art game sprite, pirate character with tricorn hat, red coat,
cutlass at hip, idle standing pose, facing right, transparent background,
64x64 pixels, clean pixel art style, no anti-aliasing, flat colors,
warm earthy palette with red accents, single frame, centered in frame,
game-ready asset
```

### Prompt 1B: Seed Refinement

Use the Phase 1A output as the input image.

```text
Refine this pixel art game sprite. Keep the character design identical.
Improve cleaner pixel edges, better color consistency, and a more defined
silhouette. Preserve the same pose, same style, same framing, and transparent
background. Do not change the character design. Only improve quality.
```

### Seed Acceptance Checklist

A seed frame is approved only if all of the following are true:

1. Silhouette is readable at small size
2. Palette is coherent and limited
3. Character is centered and properly scaled
4. No stray pixels or fuzzy edges
5. Pose is neutral enough to anchor all later animations
6. Design details are clear enough to survive animation

Save approved output as:

```text
seed/idle_frame_01.png
```

---

## Phase 2: Animation Strip Generation

For every animation below, always attach `seed/idle_frame_01.png` as the reference image.

### Prompt 2A: Idle Animation

```text
Pixel art sprite sheet, idle breathing animation, [N] frames horizontal strip,
each frame [W]x[H] pixels, transparent background, based on the reference
character provided. Subtle up-down bob motion, gentle breathing,
minimal movement. Same art style, same colors, same character as reference.
Frames evenly spaced in a single horizontal row. No gaps between frames.
```

### Example

```text
Pixel art sprite sheet, idle breathing animation, 8 frames horizontal strip,
each frame 64x64 pixels, transparent background, based on the reference
pirate character provided. Subtle up-down bob motion, gentle breathing,
coat sway. Same pixel art style, same warm earthy palette, same character.
Frames evenly spaced in a single horizontal row. No gaps between frames.
Total output 512x64 pixels.
```

### Prompt 2B: Walk Cycle

```text
Pixel art sprite sheet, walk cycle animation, [N] frames horizontal strip,
each frame [W]x[H] pixels, transparent background, based on the reference
character provided. Smooth walking motion, character moving right,
arms and legs alternating naturally. Same art style, same colors,
same character as reference. Frames evenly spaced in a single horizontal row.
No gaps between frames.
```

### Prompt 2C: Run Cycle

```text
Pixel art sprite sheet, run cycle animation, [N] frames horizontal strip,
each frame [W]x[H] pixels, transparent background, based on the reference
character provided. Fast running motion, character moving right,
exaggerated arm and leg movement, slight forward lean. Same art style,
same colors, same character as reference.
Frames evenly spaced in a single horizontal row. No gaps between frames.
```

### Prompt 2D: Hurt Reaction

```text
Pixel art sprite sheet, hurt reaction animation, [N] frames horizontal strip,
each frame [W]x[H] pixels, transparent background, based on the reference
character provided. Character recoiling from a hit, flinch backward,
brief knockback pose, then recovery. Same art style, same colors,
same character as reference.
Frames evenly spaced in a single horizontal row. No gaps between frames.
```

### Prompt 2E: Attack

```text
Pixel art sprite sheet, attack animation, [N] frames horizontal strip,
each frame [W]x[H] pixels, transparent background, based on the reference
character provided. [WEAPON TYPE] attack with wind-up, strike,
follow-through, and recovery. Dynamic motion, facing right.
Same art style, same colors, same character as reference.
Frames evenly spaced in a single horizontal row. No gaps between frames.
```

### Example

```text
Pixel art sprite sheet, sword slash attack animation, 5 frames horizontal strip,
each frame 64x64 pixels, transparent background, based on the reference
pirate character provided. Cutlass attack with raise sword, forward lunge,
slash arc, follow-through, and return to idle. Facing right.
Same pixel art style, same warm earthy palette, same character.
Frames evenly spaced in a single horizontal row. No gaps between frames.
Total output 320x64 pixels.
```

### Prompt 2F: Death

```text
Pixel art sprite sheet, death animation, [N] frames horizontal strip,
each frame [W]x[H] pixels, transparent background, based on the reference
character provided. Character falling with stagger, collapse,
and final rest on the ground. Dramatic but readable.
Same art style, same colors, same character as reference.
Frames evenly spaced in a single horizontal row. No gaps between frames.
```

### Prompt 2G: Jump

```text
Pixel art sprite sheet, jump animation, [N] frames horizontal strip,
each frame [W]x[H] pixels, transparent background, based on the reference
character provided. Jump arc with crouch or wind-up, launch, apex pose,
falling, and landing impact. Same art style, same colors,
same character as reference.
Frames evenly spaced in a single horizontal row. No gaps between frames.
```

---

## Phase 3: Validation and Post Processing

For every generated strip:

1. Verify the frame count
2. Verify total strip dimensions
3. Compare palette and silhouette against the seed frame
4. Confirm no gaps between frames
5. Slice the strip into individual frames
6. Build a final sprite atlas with metadata

---

## Python Slice Script

```python
from pathlib import Path
from PIL import Image

def slice_strip(strip_path, frame_width, frame_height, output_dir, animation_name):
    strip_path = Path(strip_path)
    output_path = Path(output_dir) / animation_name
    output_path.mkdir(parents=True, exist_ok=True)

    strip = Image.open(strip_path).convert("RGBA")

    if strip.height != frame_height:
        raise ValueError(f"Expected height {frame_height}, got {strip.height}")

    if strip.width % frame_width != 0:
        raise ValueError(
            f"Strip width {strip.width} is not divisible by frame width {frame_width}"
        )

    num_frames = strip.width // frame_width

    for i in range(num_frames):
        left = i * frame_width
        top = 0
        right = left + frame_width
        bottom = frame_height
        frame = strip.crop((left, top, right, bottom))
        frame.save(output_path / f"frame_{i+1:02d}.png")

    print(f"Sliced {num_frames} frames from {strip_path} into {output_path}")
```

---

## Additional Prompts That Improve Reliability

These are worth appending when a model tends to drift.

### Anti Drift Add On

```text
Maintain identical character proportions, silhouette, palette, costume,
and pixel density from the reference image. Do not redesign the character.
Do not add background elements. Do not add extra props.
```

### Strict Pixel Art Add On

```text
Strict pixel art only. Crisp pixel edges, no anti-aliasing,
no blur, no painterly shading, no gradients unless clearly pixel rendered.
```

### Framing Add On

```text
Character must remain fully visible within each frame and consistently centered.
Do not crop the head, feet, weapon, or accessories.
```

---

## Recommended Frame Counts

A practical default set:

1. Idle: 6 to 8 frames
2. Walk: 6 to 8 frames
3. Run: 6 to 8 frames
4. Hurt: 3 to 4 frames
5. Attack: 4 to 6 frames
6. Death: 6 to 8 frames
7. Jump: 4 to 6 frames

---

## File Structure

```text
sprite_gen/
├── seed/
│   └── idle_frame_01.png
├── strips/
│   ├── idle_strip.png
│   ├── walk_strip.png
│   ├── run_strip.png
│   ├── hurt_strip.png
│   ├── attack_strip.png
│   ├── death_strip.png
│   └── jump_strip.png
├── frames/
│   ├── idle/
│   │   ├── frame_01.png
│   │   └── ...
│   ├── walk/
│   └── ...
└── atlas/
    └── spritesheet.png
```

---

## Best Practices for an Agent

If you are feeding this into an agentic workflow, these should be explicit instructions:

1. Never overwrite the approved seed frame
2. Store every generation attempt with versioned filenames
3. Reject outputs that do not match expected strip dimensions
4. Reject outputs where frame count does not equal requested count
5. Compare newly generated strips visually or programmatically against the seed
6. Log prompt text used for every successful asset
7. Use the same descriptive style block in every prompt for consistency

---

## Optional Next Step

Add a second script that packs all animation frames into a final sprite atlas and emits JSON metadata describing:
- animation name
- frame count
- frame width
- frame height
- atlas x/y coordinates
- playback order
- loop behavior
