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
#include <map>
#include <string>
#include <vector>

#include "asserts.hpp"
#include "object_events.hpp"

namespace {
std::vector<std::string> create_object_event_names()
{
	std::vector<std::string> res;
	res.push_back("any");
	res.push_back("start_level");
	res.push_back("player_death");
	res.push_back("load");
	res.push_back("load_checkpoint");
	res.push_back("construct");
	res.push_back("create");
	res.push_back("done_create");
	res.push_back("become_active");
	res.push_back("surface_damage");
	res.push_back("enter_anim");
	res.push_back("end_anim");
	res.push_back("collide_level");
	res.push_back("collide_head");
	res.push_back("collide_feet");
	res.push_back("collide_damage");
	res.push_back("collide_side");
	res.push_back("stuck");
	res.push_back("jumped_on");
	res.push_back("get_hit");
	res.push_back("process");
	res.push_back("timer");
	res.push_back("enter_water");
	res.push_back("exit_water");
	res.push_back("change_solid_dimensions_fail");
	res.push_back("add_object_fail");
	res.push_back("change_animation_failure");
	res.push_back("die");
	res.push_back("interact");
	res.push_back("child_spawned");
	res.push_back("spawned");
	res.push_back("draw");
	res.push_back("begin_dialog");
	res.push_back("cosmic_shift");
	res.push_back("schedule_finished");
	res.push_back("outside_level");
	res.push_back("being_added");
	res.push_back("being_removed");
	res.push_back("window_resize");
	res.push_back("mouse_down");
	res.push_back("mouse_up");
	res.push_back("mouse_move");
	res.push_back("mouse_down*");
	res.push_back("mouse_up*");
	res.push_back("mouse_move*");
	res.push_back("mouse_enter");
	res.push_back("mouse_leave");
	res.push_back("click");
	res.push_back("drag");
	res.push_back("drag_start");
	res.push_back("drag_end");

	ASSERT_EQ(res.size(), NUM_OBJECT_BUILTIN_EVENT_IDS);
	return res;
}

std::vector<std::string>& object_event_names() {
	static std::vector<std::string> event_names = create_object_event_names();
	return event_names;
}

std::map<std::string, int> create_object_event_ids()
{
	std::map<std::string, int> result;
	for(int n = 0; n != object_event_names().size(); ++n) {
		result[object_event_names()[n]] = n;
	}

	return result;
}

std::map<std::string, int>& object_event_ids() {
	static std::map<std::string, int> event_ids = create_object_event_ids();
	return event_ids;
}

}

const std::string& get_object_event_str(int id)
{
	return object_event_names()[id];
}

int get_object_event_id(const std::string& str)
{
	std::map<std::string, int>::iterator itor = object_event_ids().find(str);
	if(itor != object_event_ids().end()) {
		return itor->second;
	}

	//we have to add a new entry for this new string
	object_event_ids()[str] = object_event_names().size();
	object_event_names().push_back(str);

	return object_event_names().size()-1;
}

variant_type_ptr get_object_event_arg_type(int id)
{
#define EVENT_ARG(event_id, arg_string) \
	case OBJECT_EVENT_##event_id: { \
		static const variant_type_ptr p = parse_variant_type(variant(arg_string)); \
		return p; \
	}
	switch(id) {
		EVENT_ARG(WINDOW_RESIZE, "{width: int, height: int}")
		EVENT_ARG(MOUSE_DOWN, "{mouse_x: int, mouse_y: int, mouse_button: int, world_point: [decimal, decimal, decimal]}")
		EVENT_ARG(MOUSE_UP, "{mouse_x: int, mouse_y: int, mouse_button: int, world_point: [decimal, decimal, decimal]}")
		EVENT_ARG(MOUSE_MOVE, "{mouse_x: int, mouse_y: int, mouse_button: int, world_point: [decimal, decimal, decimal]}")
		EVENT_ARG(MOUSE_DOWN_STAR, "{mouse_x: int, mouse_y: int, mouse_button: int, world_point: [decimal, decimal, decimal], handled: bool, objects_under_mouse: [custom_obj]}")
		EVENT_ARG(MOUSE_UP_STAR, "{mouse_x: int, mouse_y: int, mouse_button: int, world_point: [decimal, decimal, decimal], handled: bool, objects_under_mouse: [custom_obj]}")
		EVENT_ARG(MOUSE_MOVE_STAR, "{mouse_x: int, mouse_y: int, mouse_button: int, world_point: [decimal, decimal, decimal], handled: bool, objects_under_mouse: [custom_obj]}")
		EVENT_ARG(MOUSE_ENTER, "{mouse_x: int, mouse_y: int, mouse_button: int, world_point: [decimal, decimal, decimal]}")
		EVENT_ARG(MOUSE_LEAVE, "{mouse_x: int, mouse_y: int, mouse_button: int, world_point: [decimal, decimal, decimal]}")
		EVENT_ARG(MOUSE_CLICK, "{mouse_x: int, mouse_y: int, mouse_button: int, world_point: [decimal, decimal, decimal]}")
		EVENT_ARG(MOUSE_DRAG, "{mouse_x: int, mouse_y: int, mouse_button: int, world_point: [decimal, decimal, decimal]}")
		EVENT_ARG(MOUSE_DRAG_START, "{mouse_x: int, mouse_y: int, mouse_button: int, world_point: [decimal, decimal, decimal]}")
		EVENT_ARG(MOUSE_DRAG_END, "{mouse_x: int, mouse_y: int, mouse_button: int, world_point: [decimal, decimal, decimal]}")
		EVENT_ARG(SPAWNED, "{spawner: custom_obj, child: custom_obj}")
		EVENT_ARG(CHILD_SPAWNED, "{spawner: custom_obj, child: custom_obj}")
		EVENT_ARG(ADD_OBJECT_FAIL, "{collide_with: custom_obj|null, object: custom_obj|null}")
		EVENT_ARG(COLLIDE_HEAD, "{area: string|null, collide_with: custom_obj|null, collide_with_area: string|null}")
		EVENT_ARG(COLLIDE_FEET, "{area: string|null, collide_with: custom_obj|null, collide_with_area: string|null}")
		EVENT_ARG(COLLIDE_SIDE, "{area: string|null, collide_with: custom_obj|null, collide_with_area: string|null}")
		EVENT_ARG(CHANGE_ANIMATION_FAILURE, "{previous_animation: string}")
		EVENT_ARG(COSMIC_SHIFT, "{xshift: int, yshift: int}")
		default: {
			const std::string& str = get_object_event_str(id);
			if(strstr(str.c_str(), "collide_object")) {
				return parse_variant_type(variant("builtin user_collision_callable"));
			}

			return variant_type_ptr();
		}
	}
#undef EVENT_ARG
}
