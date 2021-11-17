#include "buffers/output_stream_buffer.h"

#include <ostream>

namespace buffers
{

output_stream_buffer::output_stream_buffer(std::ostream& stream)
    : stream_(stream)
{
    stream_.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    stream_.clear();
}

std::size_t output_stream_buffer::size()
{
    stream_.clear();
    auto old_pos = stream_.tellp();
    stream_.seekp(0, std::ios_base::end);
    auto result = stream_.tellp();
    stream_.seekp(old_pos);
    return static_cast<std::size_t>(result);
}

void output_stream_buffer::write(std::size_t count, const std::byte* data)
{
    stream_.write(reinterpret_cast<const char*>(data), count);
}

void output_stream_buffer::advance_wpos(std::int32_t offset)
{
    stream_.clear();
    stream_.seekp(offset, std::ios_base::cur);
}

void output_stream_buffer::set_wpos(std::size_t pos)
{
    stream_.clear();
    stream_.seekp(pos, std::ios_base::beg);
}

std::size_t output_stream_buffer::wpos()
{
    return static_cast<std::size_t>(stream_.tellp());
}

} //namespace buffers
