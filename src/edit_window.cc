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

#include "common.hh"
#include <SDL.h>
#include <GL/gl.h>
#include "edit.hh"
#include "gui.hh"

namespace edit {

window::window() :
	gui::window("Dr. N. Essef",800,600)
{
}

void window::frame(int delta_time)
{
	namespace im = gui::im;

	// Clear the screen.
	glClear(GL_COLOR_BUFFER_BIT);

	// Do the main menu.
	im::main_menu_bar([&]{
		im::menu("File",[&]{
			im::menu_item("Quit",[&]{
				SDL_Quit();
				std::exit(EXIT_SUCCESS);
			});
		});
	});
}

}
