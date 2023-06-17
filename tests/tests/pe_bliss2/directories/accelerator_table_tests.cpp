#include "gtest/gtest.h"

#include "pe_bliss2/detail/resources/accelerator.h"
#include "pe_bliss2/resources/accelerator_table.h"

using namespace pe_bliss::resources;

TEST(AcceleratorTableTests, VirtualKeyCodeToString)
{
	EXPECT_EQ(virtual_key_code_to_string(virtual_key_code::accept), "accept");
	EXPECT_EQ(virtual_key_code_to_string(virtual_key_code::key_5), "5");
	EXPECT_EQ(virtual_key_code_to_string(virtual_key_code::key_return), "return");
	EXPECT_EQ(virtual_key_code_to_string(virtual_key_code::key_delete), "delete");
	EXPECT_EQ(virtual_key_code_to_string(virtual_key_code::key_p), "p");
	EXPECT_EQ(virtual_key_code_to_string(static_cast<virtual_key_code>(0)), "Unknown");
}

TEST(AcceleratorTableTests, GetKeyModifiers)
{
	accelerator accel;
	EXPECT_EQ(accel.get_key_modifiers(), 0);

	accel.get_descriptor()->modifier = pe_bliss::detail::resources::modifier_alt
		| pe_bliss::detail::resources::modifier_control
		| pe_bliss::detail::resources::modifier_last_accelerator;
	EXPECT_EQ(accel.get_key_modifiers(), key_modifier::alt | key_modifier::ctrl);
}

TEST(AcceleratorTableTests, GetKeyCode)
{
	accelerator accel;
	EXPECT_EQ(accel.get_key_code(), accelerator::key_code_type{ acsii_key_code{'\0'} });

	accel.get_descriptor()->key_code = acsii_key_code{ 'c' };
	EXPECT_EQ(accel.get_key_code(), accelerator::key_code_type{ acsii_key_code{'c'} });

	accel.get_descriptor()->key_code = pe_bliss::detail::resources::vk_accept;
	accel.get_descriptor()->modifier = pe_bliss::detail::resources::modifier_virtkey;
	EXPECT_EQ(accel.get_key_code(), accelerator::key_code_type{ virtual_key_code::accept });
}
