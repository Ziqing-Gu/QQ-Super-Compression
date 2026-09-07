# Runtime base asset

Built-in image generation tool; one targeted edit of the approved reference.
The approved source render is `approved-lit.png`; the companion image is `unlit-base.png`.
No external image API, Figma, or extra material variants were used.

The unlit edit preserved the broad geometry but changed some face microtexture. The runtime therefore keeps the approved face material stationary, uses the unlit companion beneath the light layer, and removes the original baked pointer before drawing the moving indicator. These are runtime compositing operations, not a rotating single flattened picture.

## Exact edit prompt

Use case: precise-object-edit.
Input image 1 is the edit target: the approved satin-metal audio knob render.
Produce a perfectly registered UNLIT BASE texture of this exact image for a real interactive UI control. Change only the powered lights: switch the orange horseshoe lamp completely OFF, remove all its emitted warm-white light and orange spill/reflections from the receiving panel and metal, and remove the little diagonal luminous pointer entirely by restoring the uninterrupted satin face underneath. The inactive horseshoe may remain as a very subtle ivory physical channel, but there is zero light emission anywhere.
CRITICAL INVARIANTS: Keep the same image dimensions, exact knob position, diameter, perspective, bevel geometry, metallic microtexture, pale champagne/warm grey materials, fixed upper-left ambient lighting, bottom cast shadow, panel and framing. Do not move, resize, recenter or redesign anything. The circular metallic face, solid lower sidewall, thin bevel, soft heavy shadow and ivory panel must remain the same object. No indicator notch or mark; the pointer will be a separate live layer. Do not add new parts, rings, labels or text. This is an aligned OFF-state companion asset, not a new product illustration.
