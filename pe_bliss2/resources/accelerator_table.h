#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "pe_bliss2/detail/resources/accelerator.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/packed_struct.h"

namespace pe_bliss::resources
{

enum class virtual_key_code : std::uint8_t
{
	lbutton = detail::resources::vk_lbutton,
	rbutton = detail::resources::vk_rbutton,
	cancel = detail::resources::vk_cancel,
	mbutton = detail::resources::vk_mbutton,
	xbutton1 = detail::resources::vk_xbutton1,
	xbutton2 = detail::resources::vk_xbutton2,
	back = detail::resources::vk_back,
	tab = detail::resources::vk_tab,
	clear = detail::resources::vk_clear,
	key_return = detail::resources::vk_return,
	shift = detail::resources::vk_shift,
	control = detail::resources::vk_control,
	menu = detail::resources::vk_menu,
	pause = detail::resources::vk_pause,
	capital = detail::resources::vk_capital,
	kana = detail::resources::vk_kana,
	ime_on = detail::resources::vk_ime_on,
	junja = detail::resources::vk_junja,
	final = detail::resources::vk_final,
	hanja = detail::resources::vk_hanja,
	ime_off = detail::resources::vk_ime_off,
	escape = detail::resources::vk_escape,
	convert = detail::resources::vk_convert,
	nonconvert = detail::resources::vk_nonconvert,
	accept = detail::resources::vk_accept,
	modechange = detail::resources::vk_modechange,
	space = detail::resources::vk_space,
	prior = detail::resources::vk_prior,
	next = detail::resources::vk_next,
	end = detail::resources::vk_end,
	home = detail::resources::vk_home,
	left = detail::resources::vk_left,
	up = detail::resources::vk_up,
	right = detail::resources::vk_right,
	down = detail::resources::vk_down,
	select = detail::resources::vk_select,
	print = detail::resources::vk_print,
	execute = detail::resources::vk_execute,
	snapshot = detail::resources::vk_snapshot,
	insert = detail::resources::vk_insert,
	key_delete = detail::resources::vk_delete,
	help = detail::resources::vk_help,
	key_0 = '0',
	key_1, key_2, key_3, key_4, key_5, key_6, key_7, key_8, key_9,
	key_a = 'A',
	key_b, key_c, key_d, key_e, key_f, key_g, key_h, key_i, key_j,
	key_k, key_l, key_m, key_n, key_o, key_p, key_q, key_r, key_s,
	key_t, key_u, key_v, key_w, key_x, key_y, key_z,
	lwin = detail::resources::vk_lwin,
	rwin = detail::resources::vk_rwin,
	apps = detail::resources::vk_apps,
	sleep = detail::resources::vk_sleep,
	numpad0 = detail::resources::vk_numpad0,
	numpad1 = detail::resources::vk_numpad1,
	numpad2 = detail::resources::vk_numpad2,
	numpad3 = detail::resources::vk_numpad3,
	numpad4 = detail::resources::vk_numpad4,
	numpad5 = detail::resources::vk_numpad5,
	numpad6 = detail::resources::vk_numpad6,
	numpad7 = detail::resources::vk_numpad7,
	numpad8 = detail::resources::vk_numpad8,
	numpad9 = detail::resources::vk_numpad9,
	multiply = detail::resources::vk_multiply,
	add = detail::resources::vk_add,
	separator = detail::resources::vk_separator,
	subtract = detail::resources::vk_subtract,
	decimal = detail::resources::vk_decimal,
	divide = detail::resources::vk_divide,
	f1 = detail::resources::vk_f1,
	f2 = detail::resources::vk_f2,
	f3 = detail::resources::vk_f3,
	f4 = detail::resources::vk_f4,
	f5 = detail::resources::vk_f5,
	f6 = detail::resources::vk_f6,
	f7 = detail::resources::vk_f7,
	f8 = detail::resources::vk_f8,
	f9 = detail::resources::vk_f9,
	f10 = detail::resources::vk_f10,
	f11 = detail::resources::vk_f11,
	f12 = detail::resources::vk_f12,
	f13 = detail::resources::vk_f13,
	f14 = detail::resources::vk_f14,
	f15 = detail::resources::vk_f15,
	f16 = detail::resources::vk_f16,
	f17 = detail::resources::vk_f17,
	f18 = detail::resources::vk_f18,
	f19 = detail::resources::vk_f19,
	f20 = detail::resources::vk_f20,
	f21 = detail::resources::vk_f21,
	f22 = detail::resources::vk_f22,
	f23 = detail::resources::vk_f23,
	f24 = detail::resources::vk_f24,
	navigation_view = detail::resources::vk_navigation_view,
	navigation_menu = detail::resources::vk_navigation_menu,
	navigation_up = detail::resources::vk_navigation_up,
	navigation_down = detail::resources::vk_navigation_down,
	navigation_left = detail::resources::vk_navigation_left,
	navigation_right = detail::resources::vk_navigation_right,
	navigation_accept = detail::resources::vk_navigation_accept,
	navigation_cancel = detail::resources::vk_navigation_cancel,
	numlock = detail::resources::vk_numlock,
	scroll = detail::resources::vk_scroll,
	oem_fj_jisho = detail::resources::vk_oem_fj_jisho,
	oem_fj_masshou = detail::resources::vk_oem_fj_masshou,
	oem_fj_touroku = detail::resources::vk_oem_fj_touroku,
	oem_fj_loya = detail::resources::vk_oem_fj_loya,
	oem_fj_roya = detail::resources::vk_oem_fj_roya,
	lshift = detail::resources::vk_lshift,
	rshift = detail::resources::vk_rshift,
	lcontrol = detail::resources::vk_lcontrol,
	rcontrol = detail::resources::vk_rcontrol,
	lmenu = detail::resources::vk_lmenu,
	rmenu = detail::resources::vk_rmenu,
	browser_back = detail::resources::vk_browser_back,
	browser_forward = detail::resources::vk_browser_forward,
	browser_refresh = detail::resources::vk_browser_refresh,
	browser_stop = detail::resources::vk_browser_stop,
	browser_search = detail::resources::vk_browser_search,
	browser_favorites = detail::resources::vk_browser_favorites,
	browser_home = detail::resources::vk_browser_home,
	volume_mute = detail::resources::vk_volume_mute,
	volume_down = detail::resources::vk_volume_down,
	volume_up = detail::resources::vk_volume_up,
	media_next_track = detail::resources::vk_media_next_track,
	media_prev_track = detail::resources::vk_media_prev_track,
	media_stop = detail::resources::vk_media_stop,
	media_play_pause = detail::resources::vk_media_play_pause,
	launch_mail = detail::resources::vk_launch_mail,
	launch_media_select = detail::resources::vk_launch_media_select,
	launch_app1 = detail::resources::vk_launch_app1,
	launch_app2 = detail::resources::vk_launch_app2,
	oem_1 = detail::resources::vk_oem_1,
	oem_plus = detail::resources::vk_oem_plus,
	oem_comma = detail::resources::vk_oem_comma,
	oem_minus = detail::resources::vk_oem_minus,
	oem_period = detail::resources::vk_oem_period,
	oem_2 = detail::resources::vk_oem_2,
	oem_3 = detail::resources::vk_oem_3,
	gamepad_a = detail::resources::vk_gamepad_a,
	gamepad_b = detail::resources::vk_gamepad_b,
	gamepad_x = detail::resources::vk_gamepad_x,
	gamepad_y = detail::resources::vk_gamepad_y,
	gamepad_right_shoulder = detail::resources::vk_gamepad_right_shoulder,
	gamepad_left_shoulder = detail::resources::vk_gamepad_left_shoulder,
	gamepad_left_trigger = detail::resources::vk_gamepad_left_trigger,
	gamepad_right_trigger = detail::resources::vk_gamepad_right_trigger,
	gamepad_dpad_up = detail::resources::vk_gamepad_dpad_up,
	gamepad_dpad_down = detail::resources::vk_gamepad_dpad_down,
	gamepad_dpad_left = detail::resources::vk_gamepad_dpad_left,
	gamepad_dpad_right = detail::resources::vk_gamepad_dpad_right,
	gamepad_menu = detail::resources::vk_gamepad_menu,
	gamepad_view = detail::resources::vk_gamepad_view,
	gamepad_left_thumbstick_button = detail::resources::vk_gamepad_left_thumbstick_button,
	gamepad_right_thumbstick_button = detail::resources::vk_gamepad_right_thumbstick_button,
	gamepad_left_thumbstick_up = detail::resources::vk_gamepad_left_thumbstick_up,
	gamepad_left_thumbstick_down = detail::resources::vk_gamepad_left_thumbstick_down,
	gamepad_left_thumbstick_right = detail::resources::vk_gamepad_left_thumbstick_right,
	gamepad_left_thumbstick_left = detail::resources::vk_gamepad_left_thumbstick_left,
	gamepad_right_thumbstick_up = detail::resources::vk_gamepad_right_thumbstick_up,
	gamepad_right_thumbstick_down = detail::resources::vk_gamepad_right_thumbstick_down,
	gamepad_right_thumbstick_right = detail::resources::vk_gamepad_right_thumbstick_right,
	gamepad_right_thumbstick_left = detail::resources::vk_gamepad_right_thumbstick_left,
	oem_4 = detail::resources::vk_oem_4,
	oem_5 = detail::resources::vk_oem_5,
	oem_6 = detail::resources::vk_oem_6,
	oem_7 = detail::resources::vk_oem_7,
	oem_8 = detail::resources::vk_oem_8,
	oem_ax = detail::resources::vk_oem_ax,
	oem_102 = detail::resources::vk_oem_102,
	ico_help = detail::resources::vk_ico_help,
	ico_00 = detail::resources::vk_ico_00,
	processkey = detail::resources::vk_processkey,
	ico_clear = detail::resources::vk_ico_clear,
	packet = detail::resources::vk_packet,
	oem_reset = detail::resources::vk_oem_reset,
	oem_jump = detail::resources::vk_oem_jump,
	oem_pa1 = detail::resources::vk_oem_pa1,
	oem_pa2 = detail::resources::vk_oem_pa2,
	oem_pa3 = detail::resources::vk_oem_pa3,
	oem_wsctrl = detail::resources::vk_oem_wsctrl,
	oem_cusel = detail::resources::vk_oem_cusel,
	oem_attn = detail::resources::vk_oem_attn,
	oem_finish = detail::resources::vk_oem_finish,
	oem_copy = detail::resources::vk_oem_copy,
	oem_auto = detail::resources::vk_oem_auto,
	oem_enlw = detail::resources::vk_oem_enlw,
	oem_backtab = detail::resources::vk_oem_backtab,
	attn = detail::resources::vk_attn,
	crsel = detail::resources::vk_crsel,
	exsel = detail::resources::vk_exsel,
	ereof = detail::resources::vk_ereof,
	play = detail::resources::vk_play,
	zoom = detail::resources::vk_zoom,
	noname = detail::resources::vk_noname,
	pa1 = detail::resources::vk_pa1,
	oem_clear = detail::resources::vk_oem_clear,
};

using acsii_key_code = std::uint8_t;

struct key_modifier final
{
	enum value
	{
		virtkey = detail::resources::modifier_virtkey,
		alt = detail::resources::modifier_alt,
		ctrl = detail::resources::modifier_control,
		shift = detail::resources::modifier_shift
	};
};

[[nodiscard]]
std::string_view virtual_key_code_to_string(virtual_key_code code) noexcept;

class accelerator
{
public:
	using descriptor_type = packed_struct<detail::resources::accelerator>;
	using key_code_type = std::variant<acsii_key_code, virtual_key_code>;

public:
	[[nodiscard]]
	descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	const descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	key_modifier::value get_key_modifiers() const noexcept;

	[[nodiscard]] key_code_type get_key_code() const noexcept;

private:
	descriptor_type descriptor_;
};

template<typename... Bases>
class accelerator_table_base : public Bases...
{
public:
	using accelerator_list_type = std::vector<accelerator>;

public:
	[[nodiscard]] accelerator_list_type& get_accelerators() & noexcept
	{
		return list_;
	}

	[[nodiscard]] const accelerator_list_type& get_accelerators() const& noexcept
	{
		return list_;
	}

	[[nodiscard]] accelerator_list_type get_accelerators() && noexcept
	{
		return std::move(list_);
	}

private:
	accelerator_list_type list_{};
};

using accelerator_table = accelerator_table_base<>;
using accelerator_table_details = accelerator_table_base<error_list>;

} //namespace pe_bliss::resources
