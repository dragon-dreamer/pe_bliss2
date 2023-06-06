#include "buffers/output_stream_buffer.h"

#include <cstddef>
#include <ostream>

#include "utilities/scoped_guard.h"

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
    utilities::scoped_guard guard([this, old_pos = stream_.tellp()]{
        try
        {
            stream_.seekp(old_pos);
        }
        catch (...)
        {
        }
    });
    stream_.seekp(0, std::ios_base::end);
    return static_cast<std::size_t>(stream_.tellp());
}

void output_stream_buffer::write(std::size_t count, const std::byte* data)
{
    stream_.write(reinterpret_cast<const char*>(data), count);
}

void output_stream_buffer::advance_wpos(std::int32_t offset)
{
    stream_.clear();
    try
    {
        stream_.seekp(offset, std::ios_base::cur);
    }
    catch (...)
    {
        stream_.clear();
        throw;
    }
}

void output_stream_buffer::set_wpos(std::size_t pos)
{
    stream_.clear();
    try
    {
        stream_.seekp(pos, std::ios_base::beg);
    }
    catch (...)
    {
        stream_.clear();
        throw;
    }
}

std::size_t output_stream_buffer::wpos()
{
    return static_cast<std::size_t>(stream_.tellp());
}

} //namespace buffers
