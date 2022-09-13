#include "pe_bliss2/resources/accelerator_table.h"

namespace pe_bliss::resources
{

std::string_view virtual_key_code_to_string(virtual_key_code code) noexcept
{
	using enum virtual_key_code;
	switch (code)
	{
	case lbutton: return "lbutton";
	case rbutton: return "rbutton";
	case cancel: return "cancel";
	case mbutton: return "mbutton";
	case xbutton1: return "xbutton1";
	case xbutton2: return "xbutton2";
	case back: return "back";
	case tab: return "tab";
	case clear: return "clear";
	case key_return: return "return";
	case shift: return "shift";
	case control: return "control";
	case menu: return "menu";
	case pause: return "pause";
	case capital: return "capital";
	case kana: return "kana";
	case ime_on: return "ime_on";
	case junja: return "junja";
	case final: return "final";
	case hanja: return "hanja";
	case ime_off: return "ime_off";
	case escape: return "escape";
	case convert: return "convert";
	case nonconvert: return "nonconvert";
	case accept: return "accept";
	case modechange: return "modechange";
	case space: return "space";
	case prior: return "prior";
	case next: return "next";
	case end: return "end";
	case home: return "home";
	case left: return "left";
	case up: return "up";
	case right: return "right";
	case down: return "down";
	case select: return "select";
	case print: return "print";
	case execute: return "execute";
	case snapshot: return "snapshot";
	case insert: return "insert";
	case key_delete: return "delete";
	case help: return "help";
	case key_0: return "0";
	case key_1: return "1";
	case key_2: return "2";
	case key_3: return "3";
	case key_4: return "4";
	case key_5: return "5";
	case key_6: return "6";
	case key_7: return "7";
	case key_8: return "8";
	case key_9: return "9";
	case key_a: return "a";
	case key_b: return "b";
	case key_c: return "c";
	case key_d: return "d";
	case key_e: return "e";
	case key_f: return "f";
	case key_g: return "g";
	case key_h: return "h";
	case key_i: return "i";
	case key_j: return "j";
	case key_k: return "k";
	case key_l: return "l";
	case key_m: return "m";
	case key_n: return "n";
	case key_o: return "o";
	case key_p: return "p";
	case key_q: return "q";
	case key_r: return "r";
	case key_s: return "s";
	case key_t: return "t";
	case key_u: return "u";
	case key_v: return "v";
	case key_w: return "w";
	case key_x: return "x";
	case key_y: return "y";
	case key_z: return "z";
	case lwin: return "lwin";
	case rwin: return "rwin";
	case apps: return "apps";
	case sleep: return "sleep";
	case numpad0: return "numpad0";
	case numpad1: return "numpad1";
	case numpad2: return "numpad2";
	case numpad3: return "numpad3";
	case numpad4: return "numpad4";
	case numpad5: return "numpad5";
	case numpad6: return "numpad6";
	case numpad7: return "numpad7";
	case numpad8: return "numpad8";
	case numpad9: return "numpad9";
	case multiply: return "multiply";
	case add: return "add";
	case separator: return "separator";
	case subtract: return "subtract";
	case decimal: return "decimal";
	case divide: return "divide";
	case f1: return "f1";
	case f2: return "f2";
	case f3: return "f3";
	case f4: return "f4";
	case f5: return "f5";
	case f6: return "f6";
	case f7: return "f7";
	case f8: return "f8";
	case f9: return "f9";
	case f10: return "f10";
	case f11: return "f11";
	case f12: return "f12";
	case f13: return "f13";
	case f14: return "f14";
	case f15: return "f15";
	case f16: return "f16";
	case f17: return "f17";
	case f18: return "f18";
	case f19: return "f19";
	case f20: return "f20";
	case f21: return "f21";
	case f22: return "f22";
	case f23: return "f23";
	case f24: return "f24";
	case navigation_view: return "navigation_view";
	case navigation_menu: return "navigation_menu";
	case navigation_up: return "navigation_up";
	case navigation_down: return "navigation_down";
	case navigation_left: return "navigation_left";
	case navigation_right: return "navigation_right";
	case navigation_accept: return "navigation_accept";
	case navigation_cancel: return "navigation_cancel";
	case numlock: return "numlock";
	case scroll: return "scroll";
	case oem_fj_jisho: return "oem_fj_jisho";
	case oem_fj_masshou: return "oem_fj_masshou";
	case oem_fj_touroku: return "oem_fj_touroku";
	case oem_fj_loya: return "oem_fj_loya";
	case oem_fj_roya: return "oem_fj_roya";
	case lshift: return "lshift";
	case rshift: return "rshift";
	case lcontrol: return "lcontrol";
	case rcontrol: return "rcontrol";
	case lmenu: return "lmenu";
	case rmenu: return "rmenu";
	case browser_back: return "browser_back";
	case browser_forward: return "browser_forward";
	case browser_refresh: return "browser_refresh";
	case browser_stop: return "browser_stop";
	case browser_search: return "browser_search";
	case browser_favorites: return "browser_favorites";
	case browser_home: return "browser_home";
	case volume_mute: return "volume_mute";
	case volume_down: return "volume_down";
	case volume_up: return "volume_up";
	case media_next_track: return "media_next_track";
	case media_prev_track: return "media_prev_track";
	case media_stop: return "media_stop";
	case media_play_pause: return "media_play_pause";
	case launch_mail: return "launch_mail";
	case launch_media_select: return "launch_media_select";
	case launch_app1: return "launch_app1";
	case launch_app2: return "launch_app2";
	case oem_1: return "oem_1";
	case oem_plus: return "oem_plus";
	case oem_comma: return "oem_comma";
	case oem_minus: return "oem_minus";
	case oem_period: return "oem_period";
	case oem_2: return "oem_2";
	case oem_3: return "oem_3";
	case gamepad_a: return "gamepad_a";
	case gamepad_b: return "gamepad_b";
	case gamepad_x: return "gamepad_x";
	case gamepad_y: return "gamepad_y";
	case gamepad_right_shoulder: return "gamepad_right_shoulder";
	case gamepad_left_shoulder: return "gamepad_left_shoulder";
	case gamepad_left_trigger: return "gamepad_left_trigger";
	case gamepad_right_trigger: return "gamepad_right_trigger";
	case gamepad_dpad_up: return "gamepad_dpad_up";
	case gamepad_dpad_down: return "gamepad_dpad_down";
	case gamepad_dpad_left: return "gamepad_dpad_left";
	case gamepad_dpad_right: return "gamepad_dpad_right";
	case gamepad_menu: return "gamepad_menu";
	case gamepad_view: return "gamepad_view";
	case gamepad_left_thumbstick_button: return "gamepad_left_thumbstick_button";
	case gamepad_right_thumbstick_button: return "gamepad_right_thumbstick_button";
	case gamepad_left_thumbstick_up: return "gamepad_left_thumbstick_up";
	case gamepad_left_thumbstick_down: return "gamepad_left_thumbstick_down";
	case gamepad_left_thumbstick_right: return "gamepad_left_thumbstick_right";
	case gamepad_left_thumbstick_left: return "gamepad_left_thumbstick_left";
	case gamepad_right_thumbstick_up: return "gamepad_right_thumbstick_up";
	case gamepad_right_thumbstick_down: return "gamepad_right_thumbstick_down";
	case gamepad_right_thumbstick_right: return "gamepad_right_thumbstick_right";
	case gamepad_right_thumbstick_left: return "gamepad_right_thumbstick_left";
	case oem_4: return "oem_4";
	case oem_5: return "oem_5";
	case oem_6: return "oem_6";
	case oem_7: return "oem_7";
	case oem_8: return "oem_8";
	case oem_ax: return "oem_ax";
	case oem_102: return "oem_102";
	case ico_help: return "ico_help";
	case ico_00: return "ico_00";
	case processkey: return "processkey";
	case ico_clear: return "ico_clear";
	case packet: return "packet";
	case oem_reset: return "oem_reset";
	case oem_jump: return "oem_jump";
	case oem_pa1: return "oem_pa1";
	case oem_pa2: return "oem_pa2";
	case oem_pa3: return "oem_pa3";
	case oem_wsctrl: return "oem_wsctrl";
	case oem_cusel: return "oem_cusel";
	case oem_attn: return "oem_attn";
	case oem_finish: return "oem_finish";
	case oem_copy: return "oem_copy";
	case oem_auto: return "oem_auto";
	case oem_enlw: return "oem_enlw";
	case oem_backtab: return "oem_backtab";
	case attn: return "attn";
	case crsel: return "crsel";
	case exsel: return "exsel";
	case ereof: return "ereof";
	case play: return "play";
	case zoom: return "zoom";
	case noname: return "noname";
	case pa1: return "pa1";
	case oem_clear: return "oem_clear";
	default:
		return "Unknown";
	}
}

key_modifier::value accelerator::get_key_modifiers() const noexcept
{
	return static_cast<key_modifier::value>(descriptor_->modifier
		& ~detail::resources::modifier_last_accelerator);
}

accelerator::key_code_type accelerator::get_key_code() const noexcept
{
	if (descriptor_->modifier & key_modifier::virtkey)
		return static_cast<virtual_key_code>(descriptor_->key_code);

	return static_cast<acsii_key_code>(descriptor_->key_code);
}

} //namespace pe_bliss::resources
