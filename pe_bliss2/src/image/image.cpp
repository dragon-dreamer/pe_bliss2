#include "pe_bliss2/image/image.h"

#include <cstdint>
#include <limits>
#include <utility>

#include "buffers/output_memory_buffer.h"

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/detail/image_file_header.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/pe_error.h"

#include "utilities/generic_error.h"
#include "utilities/math.h"

namespace pe_bliss::image
{

bool image::has_relocations() const noexcept
{
	return data_directories_.has_reloc()
		&& !(file_header_.get_characteristics()
			& core::file_header::characteristics::relocs_stripped);
}

bool image::is_64bit() const noexcept
{
	return optional_header_.get_magic() == core::optional_header::magic::pe64;
}

void image::update_number_of_sections()
{
	auto number_of_sections = section_table_.get_section_headers().size();
	using number_of_sections_type = decltype(detail::image_file_header::number_of_sections);
	if (number_of_sections > (std::numeric_limits<number_of_sections_type>::max)())
		throw pe_error(image_errc::too_many_sections);

	file_header_.get_descriptor()->number_of_sections
		= static_cast<number_of_sections_type>(number_of_sections);
}

void image::set_number_of_data_directories(std::uint32_t number)
{
	if (number > core::optional_header::max_number_of_rva_and_sizes)
		throw pe_error(image_errc::too_many_rva_and_sizes);

	data_directories_.set_size(number);
	optional_header_.set_raw_number_of_rva_and_sizes(number);
}

void image::update_image_size()
{
	if (section_table_.get_section_headers().empty())
	{
		optional_header_.set_raw_size_of_image(
			optional_header_.get_raw_size_of_headers());
	}
	else
	{
		const auto& last_section = section_table_.get_section_headers().back();
		auto size_of_image = last_section.get_rva();
		if (!utilities::math::add_if_safe(size_of_image,
			last_section.get_virtual_size(optional_header_.get_raw_section_alignment())))
		{
			throw pe_error(utilities::generic_errc::integer_overflow);
		}
		optional_header_.set_raw_size_of_image(size_of_image);
	}
}

std::uint32_t image::strip_data_directories(std::uint32_t min_count)
{
	auto result = data_directories_.strip_data_directories(min_count);
	optional_header_.set_raw_number_of_rva_and_sizes(result);
	return result;
}

void image::copy_referenced_section_memory()
{
	for (auto& section : section_list_)
		section.copy_referenced_buffer();
}

void image::update_full_headers_buffer(bool keep_headers_gap_data)
{
	if (!keep_headers_gap_data)
		full_headers_buffer_ = {};

	buffers::output_memory_buffer::buffer_type data(std::move(
		full_headers_buffer_.copied_data()));

	buffers::output_memory_buffer buffer(data);
	dos_header_.serialize(buffer);
	dos_stub_.serialize(buffer);
	buffer.set_wpos(dos_header_.get_descriptor()->e_lfanew);
	image_signature_.serialize(buffer);
	file_header_.serialize(buffer);
	auto optional_header_buffer_pos = buffer.wpos();
	optional_header_.serialize(buffer);
	data_directories_.serialize(buffer);

	buffer.set_wpos(optional_header_buffer_pos
		+ file_header_.get_descriptor()->size_of_optional_header);
	section_table_.serialize(buffer);

	data.resize(optional_header_.get_raw_size_of_headers());

	full_headers_buffer_.copied_data() = std::move(data);
}

/*
//TODO
//Prepares section before attaching it
void pe_base::prepare_section(section& s)
{
	//Calculate its size of raw data
	s.set_size_of_raw_data(static_cast<uint32_t>(pe_utils::align_up(s.get_raw_data().length(), get_file_alignment())));

	//Check section virtual and raw size
	if(!s.get_size_of_raw_data() && !s.get_virtual_size())
		throw pe_exception("Virtual and Physical sizes of section can't be 0 at the same time", pe_exception::zero_section_sizes);

	//If section virtual size is zero
	if(!s.get_virtual_size())
	{
		s.set_virtual_size(s.get_size_of_raw_data());
	}
	else
	{
		//Else calculate its virtual size
		s.set_virtual_size(
			std::max<uint32_t>(pe_utils::align_up(s.get_size_of_raw_data(), get_file_alignment()),
			pe_utils::align_up(s.get_virtual_size(), get_section_alignment())));
	}
}

//Adds section to image
section& pe_base::add_section(section s)
{
	if(sections_.size() >= maximum_number_of_sections)
		throw pe_exception("Maximum number of sections has been reached", pe_exception::no_more_sections_can_be_added);

	//Prepare section before adding it
	prepare_section(s);

	//Calculate section virtual address
	if(!sections_.empty())
	{
		s.set_virtual_address(pe_utils::align_up(sections_.back().get_virtual_address() + sections_.back().get_aligned_virtual_size(get_section_alignment()), get_section_alignment()));

		//We should align last section raw size, if it wasn't aligned
		section& last = sections_.back();
		last.set_size_of_raw_data(static_cast<uint32_t>(pe_utils::align_up(last.get_raw_data().length(), get_file_alignment())));
	}
	else
	{
		s.set_virtual_address(
			s.get_virtual_address() == 0
			? pe_utils::align_up(get_size_of_headers(), get_section_alignment())
			: pe_utils::align_up(s.get_virtual_address(), get_section_alignment()));
	}

	//Add section to the end of section list
	sections_.push_back(s);
	//Set number of sections in PE header
	set_number_of_sections(static_cast<uint16_t>(sections_.size()));
	//Recalculate virtual size of image
	set_size_of_image(get_size_of_image() + s.get_aligned_virtual_size(get_section_alignment()));
	//Return last section
	return sections_.back();
}

//Helper function to recalculate RAW and virtual section sizes and strip it, if necessary
void pe_base::recalculate_section_sizes(section& s, bool auto_strip)
{
	prepare_section(s); //Recalculate section raw addresses

	//Strip RAW size of section, if it is the last one
	//For all others it must be file-aligned and calculated by prepare_section() call
	if(auto_strip && !(sections_.empty() || &s == &*(sections_.end() - 1)))
	{
		//Strip ending raw data nullbytes to optimize size
		std::string& raw_data = s.get_raw_data();
		if(!raw_data.empty())
		{
			std::string::size_type i = raw_data.length();
			for(; i != 1; --i)
			{
				if(raw_data[i - 1] != 0)
					break;
			}

			raw_data.resize(i);
		}

		s.set_size_of_raw_data(static_cast<uint32_t>(raw_data.length()));
	}

	//Can occur only for last section
	if(pe_utils::align_up(s.get_virtual_size(), get_section_alignment()) < pe_utils::align_up(s.get_size_of_raw_data(), get_file_alignment()))
		set_section_virtual_size(s, pe_utils::align_up(s.get_size_of_raw_data(), get_section_alignment())); //Recalculate section virtual size
}

//Realigns section by index
void pe_base::realign_section(uint32_t index)
{
	//Check index
	if(sections_.size() <= index)
		throw pe_exception("Section not found", pe_exception::section_not_found);

	//Get section iterator
	section_list::iterator it = sections_.begin() + index;
	section& s = *it;

	//Calculate, how many null bytes we have in the end of raw section data
	std::size_t strip = 0;
	for(std::size_t i = (*it).get_raw_data().length(); i >= 1; --i)
	{
		if(s.get_raw_data()[i - 1] == 0)
			strip++;
		else
			break;
	}

	if(it == sections_.end() - 1) //If we're realigning the last section
	{
		//We can strip ending null bytes
		s.set_size_of_raw_data(static_cast<uint32_t>(s.get_raw_data().length() - strip));
		s.get_raw_data().resize(s.get_raw_data().length() - strip, 0);
	}
	else
	{
		//Else just set size of raw data
		uint32_t raw_size_aligned = s.get_aligned_raw_size(get_file_alignment());
		s.set_size_of_raw_data(raw_size_aligned);
		s.get_raw_data().resize(raw_size_aligned, 0);
	}
}
*/

} //namespace pe_bliss::image
