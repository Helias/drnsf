//
// DRNSF - An unofficial Crash Bandicoot level editor
// Copyright (C) 2017  DRNSF contributors
//
// See the AUTHORS.md file for more details.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#version 150

flat in int v_Axis;
in float v_Value;

const vec4 AXIS_POS_COLORS[] = vec4[](
    vec4(1.0,0.0,0.0,1.0),
    vec4(0.0,1.0,0.0,1.0),
    vec4(0.0,0.0,1.0,1.0)
);

const vec4 AXIS_NEG_COLOR = vec4(0.5,0.5,0.5,1.0);

const vec4 CUBE_COLOR = vec4(0.1,0.1,0.1,1.0);

void main()
{
    if (v_Axis == -1) {
        gl_FragColor = CUBE_COLOR;
    } else if (v_Value >= 0.0) {
        gl_FragColor = AXIS_POS_COLORS[v_Axis];
    } else {
        gl_FragColor = AXIS_NEG_COLOR;
    }
}
