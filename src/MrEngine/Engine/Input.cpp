/*
* Viry3D
* Copyright 2014-2019 by Stack - stackos@qq.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "Input.h"
#include "memory/Memory.h"
#include "Debug.h"
#include <list>

std::vector<moonriver::Touch> g_input_touches;
std::list<moonriver::Touch> g_input_touch_buffer;
bool g_key_down[(int) moonriver::KeyCode::COUNT];
bool g_key[(int) moonriver::KeyCode::COUNT];
bool g_key_up[(int) moonriver::KeyCode::COUNT];
bool g_mouse_button_down[3];
bool g_mouse_button_up[3];
moonriver::Vector3 g_mouse_position;
bool g_mouse_button_held[3];
float g_mouse_scroll_wheel = 0;
static std::vector<unsigned short> g_input_queue_characters;

namespace moonriver
{
	bool Input::GetMouseButtonDown(int index)
	{
		return g_mouse_button_down[index];
	}

	bool Input::GetMouseButton(int index)
	{
		return g_mouse_button_held[index];
	}

	bool Input::GetMouseButtonUp(int index)
	{
		return g_mouse_button_up[index];
	}

    const Vector3& Input::GetMousePosition()
	{
		return g_mouse_position;
	}

    float Input::GetMouseScrollWheel()
    {
        return g_mouse_scroll_wheel;
    }

	int Input::GetTouchCount()
	{
		return g_input_touches.size();
	}

	const Touch& Input::GetTouch(int index)
	{
        return g_input_touches[index];
	}

	bool Input::GetKeyDown(KeyCode key)
	{
		return g_key_down[(int) key];
	}

	bool Input::GetKey(KeyCode key)
	{
		return g_key[(int) key];
	}

	bool Input::GetKeyUp(KeyCode key)
	{
		return g_key_up[(int) key];
	}

    void Input::AddInputCharacter(unsigned short c)
    {
        g_input_queue_characters.push_back(c);
    }

    const std::vector<unsigned short>& Input::GetInputQueueCharacters()
    {
        return g_input_queue_characters;
    }

	void Input::Update()
	{
		g_input_touches.clear();
		if (!g_input_touch_buffer.empty())
		{
			g_input_touches.push_back(g_input_touch_buffer.front());
			g_input_touch_buffer.pop_front();
		}

		Memory::Zero(g_key_down, sizeof(g_key_down));
		Memory::Zero(g_key_up, sizeof(g_key_up));
		Memory::Zero(g_mouse_button_down, sizeof(g_mouse_button_down));
		Memory::Zero(g_mouse_button_up, sizeof(g_mouse_button_up));
        g_mouse_scroll_wheel = 0;

        g_input_queue_characters.clear();
	}
}
