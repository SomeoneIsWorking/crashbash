# Crash Bash heap packet pools and first 2D submitter

The retail renderer does not use a fixed packet arena. Ghidra decompilation of resident functions
`0x800274FC` and `0x800276C4` shows two calls to the guest heap allocator, each for
`requested_size + 0x1800` bytes. The functions publish inclusive base and exclusive end pointers at:

| parity | base pointer global | end pointer global | current pointer global |
|---|---:|---:|---:|
| 0 | `0x8005F790` | `0x8005F794` | `0x8005F798` |
| 1 | `0x8006379C` | `0x800637A0` | `0x800637A4` |

The exact objective-frame RAM dump at replay frame 2500 contained live bounds
`[0x801F97E8,0x801FEFE8)` and `[0x801F3FD0,0x801F97D0)`. Those are observed values, not title
constants. The tracked `GameConfig` facts therefore name the six descriptor globals, never those heap
addresses.

The OTs are separate static storage. `0x800275F8` clears 4,096 entries at `0x8005B790` and
`0x8005F79C`; `0x800272AC` alternates between them, clears the selected OT, and resets its current
packet pointer from the adjacent base global. The OT-page stride is `0x400C`.

## Objective-frame attribution

Before runtime pool descriptors existed, the exact frame-2500 `otattr` run walked 6,164 OT nodes but
recorded zero writer spans. With psxport's live-pool path and the six measured globals, the same replay
records 1,828 spans. Of the 2,068 OT rows printed by the bounded REPL report, all 108 rows inside the
active direct packet pool have an emitting function:

| emitter | rows | first GPU opcode | interpretation |
|---|---:|---:|---|
| `0x8002992C` | 92 | `0x3C` | textured Gouraud quad |
| `0x80029D28` | 6 | `0x2C` | textured flat-color quad |
| `0x80018B08` | 5 | `0xE3` | draw-area/state packet path |
| `0x8001A0D8` | 5 | `0xE1` | draw-mode plus untextured Gouraud quad path |

The other 1,960 printed rows are cached/model packets outside the direct heap pools; they remain a
different producer family rather than being hidden by one broad heap window.

Ghidra decompilation of `0x8002992C` identifies the pre-packet semantic boundary. Its arguments carry
the texture descriptor, packed screen position, OT bin, and four vertex colors; the function derives
UV/CLUT/tpage state and writes a `0x34`-byte packet. This is the first native 2D producer target. Capture
those source arguments at the retained-super boundary and publish the native quad from that decoded
state. Do not consume the packet, OT, GP0 stream, VRAM output, or PSX framebuffer as product input.
