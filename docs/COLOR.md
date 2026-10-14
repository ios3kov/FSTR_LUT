# Research color contract

Borrowed CPU interleaved RGBA float32; positive stride in float elements; declared accessible length; full frame; caller owns memory through render. Explicit sRGB/Rec.709 D65 primaries and sRGB or linear transfer. AE input-space determination is pending; no inference from file or host.

Model: experimental FFDNet Color Clip; RGB NCHW encoded sRGB [0,1]; sigma in encoded sample units (15/255 research initial). Linear input converts exactly once to encoded sRGB and model result back once. AI supports opaque input only now. Nonfinite, negative, above-one or nonopaque inputs produce an explicit error, never a successful clipped frame. V3 alpha/extended support remains open. Mix=0 copies original exactly and skips model. Enhance=0 leaves LUT usable.

Replicate bottom/right to even size; channel c*4+dy*2+dx; thirteenth plane sigma; Conv/ReLU trunk outputs 12 planes; inverse shuffle/crop. FP32, no tiling/quantization/auto color/skin detector. Do not clamp model output.

LUT: encoded sRGB, red fastest, tetrahedral. Clamp lookup coordinate only and add original out-of-domain residual to output. Boundary-residual continuation is not slope extrapolation or arbitrary-LUT correctness; identity remains identity outside. Blend Look Amount and overall Mix in linear light for explicit sRGB/linear-sRGB.
Preserve stored alpha; LUT-only path supports explicit unpremultiply/re-premultiply. Alpha=0 hidden RGB unchanged. AI transparency is not implemented.
Neutral v1 identity. Warm Film v1 is original analytical research art direction, not exact film emulation or final accepted look.
