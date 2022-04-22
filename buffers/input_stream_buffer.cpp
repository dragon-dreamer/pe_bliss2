#include "buffers/input_stream_buffer.h"

#include <istream>
#include <utility>

#include "utilities/scoped_guard.h"

namespace buffers
{

input_stream_buffer::input_stream_buffer(std::shared_ptr<std::istream> stream)
    : stream_(std::move(stream))
{
    stream_->exceptions(std::ios_base::badbit | std::ios_base::failbit);
    stream_->clear();
}

std::size_t input_stream_buffer::size()
{
    stream_->clear();
    utilities::scoped_guard guard([this, old_pos = stream_->tellg()] {
        stream_->seekg(old_pos);
    });
    stream_->seekg(0, std::ios_base::end);
    return static_cast<std::size_t>(stream_->tellg());
}

std::size_t input_stream_buffer::read(std::size_t count, std::byte* data)
{
    static_assert(sizeof(std::byte) == sizeof(char));

    stream_->exceptions(std::ios_base::badbit);
    utilities::scoped_guard guard([this] {
        stream_->exceptions(std::ios_base::badbit | std::ios_base::failbit);
    });
    stream_->read(reinterpret_cast<char*>(data), count);
    if (stream_->eof())
        stream_->clear();
    return static_cast<std::size_t>(stream_->gcount());
}

void input_stream_buffer::advance_rpos(std::int32_t offset)
{
    stream_->clear();
    try
    {
        stream_->seekg(offset, std::ios_base::cur);
    }
    catch (...)
    {
        stream_->clear();
        throw;
    }
}

void input_stream_buffer::set_rpos(std::size_t pos)
{
    stream_->clear();
    try
    {
        stream_->seekg(pos, std::ios_base::beg);
    }
    catch (...)
    {
        stream_->clear();
        throw;
    }
}

std::size_t input_stream_buffer::rpos()
{
    return static_cast<std::size_t>(stream_->tellg());
}

} //namespace buffers
