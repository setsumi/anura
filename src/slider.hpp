/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef SLIDER_HPP_INCLUDED
#define SLIDER_HPP_INCLUDED

#include <boost/function.hpp>

#include "image_widget.hpp"
#include "texture.hpp"
#include "widget.hpp"
#include "gui_section.hpp"


namespace gui {
	
//A slider widget. Forwards to a given function whenever its value changes.
class slider : public widget
{
public:
	explicit slider(int width, boost::function<void (double)> onchange, double position=0.0, int scale=2);
	explicit slider(const variant& v, game_logic::formula_callable* e);
	double position() const {return position_;};
	void set_position (double position) {position_ = position;};
	void set_drag_end(boost::function<void (double)> ondragend) { ondragend_ = ondragend; }
		
protected:
	virtual void set_value(const std::string& key, const variant& v);
	virtual variant get_value(const std::string& key) const;
	void init() const;

private:
	bool in_slider(int x, int y) const;
	bool in_button(int x, int y) const;
	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);
	void handle_process();
		
	int width_;
	boost::function<void (double)> onchange_;
	boost::function<void (double)> ondragend_;
	bool dragging_;
	double position_;
		
	widget_ptr slider_left_, slider_right_, slider_middle_, slider_button_;

	game_logic::formula_ptr ffl_handler_;
	void change_delegate(double);
	game_logic::formula_ptr ffl_end_handler_;
	void dragend_delegate(double);
};
	
typedef boost::intrusive_ptr<slider> slider_ptr;
	
}

#endif
