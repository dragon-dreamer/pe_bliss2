#include "buffers/input_stream_buffer.h"

#include <istream>

namespace buffers
{

input_stream_buffer::input_stream_buffer(const std::shared_ptr<std::istream>& stream)
    : stream_(stream)
{
    stream_->exceptions(std::ios_base::badbit | std::ios_base::failbit);
    stream_->clear();
}

std::size_t input_stream_buffer::size()
{
    stream_->clear();
    auto old_pos = stream_->tellg();
    stream_->seekg(0, std::ios_base::end);
    auto result = stream_->tellg();
    stream_->seekg(old_pos);
    return static_cast<std::size_t>(result);
}

std::size_t input_stream_buffer::read(std::size_t count, std::byte* data)
{
    stream_->exceptions(std::ios_base::badbit);
    stream_->read(reinterpret_cast<char*>(data), count);
    if (stream_->eof())
        stream_->clear();
    stream_->exceptions(std::ios_base::badbit | std::ios_base::failbit);
    return static_cast<std::size_t>(stream_->gcount());
}

void input_stream_buffer::advance_rpos(std::int32_t offset)
{
    stream_->clear();
    stream_->seekg(offset, std::ios_base::cur);
}

void input_stream_buffer::set_rpos(std::size_t pos)
{
    stream_->clear();
    stream_->seekg(pos, std::ios_base::beg);
}

std::size_t input_stream_buffer::rpos()
{
    return static_cast<std::size_t>(stream_->tellg());
}

} //namespace buffers
