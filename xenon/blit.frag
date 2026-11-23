#version 450
// The game's frame, its palette already turned into colours on the CPU, drawn with Settings > Smoothing:
//   off: point sampled (with a point sampler), each of the game's pixels a hard block;
//   smooth: sampled linearly, blending each pixel into the next;
//   sharp: sampled linearly only across the screen pixels where one of the game's pixels meets the next, so the
//     blocks keep their edges, softened by a screen pixel or so, and are all the same size ("sharp bilinear").
// pc.size: the frame's width and height in pixels, then how many screen pixels each takes across and down (1 for off
// and smooth, which then sample where they're given).
layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o_color;
layout(set = 0, binding = 0) uniform sampler2D u_frame;
layout(push_constant) uniform Push { vec4 size; } pc;

void main()
{
	vec2 texel = v_uv * pc.size.xy;
	vec2 offset = fract(texel) - 0.5;
	vec2 flat_half = 0.5 - 0.5 / pc.size.zw;
	vec2 blend = (offset - min(max(offset, -flat_half), flat_half)) * pc.size.zw;
	o_color = texture(u_frame, (floor(texel) + 0.5 + blend) / pc.size.xy);
}
