#version 450
// A corner of the quad Heretic's frame is drawn on: where it sits, from -1 to 1 across the screen and up it, and where
// it reads the frame's texture, v = 0 its top row.
layout(location = 0) in vec2 a_position;
layout(location = 2) in vec2 a_uv;
layout(location = 0) out vec2 v_uv;

void main()
{
	gl_Position = vec4(a_position, 0.0, 1.0);
	v_uv = a_uv;
}
