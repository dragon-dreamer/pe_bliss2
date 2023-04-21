#include "buffers/input_stream_buffer.h"

#include <istream>
#include <utility>

#include "utilities/generic_error.h"
#include "utilities/math.h"
#include "utilities/scoped_guard.h"

namespace buffers
{
input_stream_buffer::input_stream_buffer(std::shared_ptr<std::istream> stream)
    : stream_(std::move(stream))
    , size_{}
{
    stream_->clear();
    stream_->exceptions(std::ios_base::badbit | std::ios_base::failbit);

    utilities::scoped_guard guard([this, old_pos = stream_->tellg()]{
        try
        {
            stream_->seekg(old_pos);
        }
        catch (...)
        {
        }
    });
    stream_->seekg(0, std::ios_base::end);
    size_ = static_cast<std::size_t>(stream_->tellg());
}

std::size_t input_stream_buffer::size()
{
    return size_;
}

std::size_t input_stream_buffer::read(std::size_t pos,
    std::size_t count, std::byte* data)
{
    static_assert(sizeof(std::byte) == sizeof(char));

    if (!count)
        return 0u;

    if (!utilities::math::is_sum_safe(pos, count) || pos + count > size_)
        throw std::system_error(utilities::generic_errc::buffer_overrun);

    stream_->exceptions(std::ios_base::badbit);
    utilities::scoped_guard guard([this] {
        try
        {
            stream_->exceptions(std::ios_base::badbit | std::ios_base::failbit);
        }
        catch (...)
        {
        }
    });
    stream_->seekg(pos, std::ios::beg);
    stream_->read(reinterpret_cast<char*>(data), count);
    if (stream_->eof())
        stream_->clear();
    if (static_cast<std::size_t>(stream_->gcount()) != count)
        throw std::system_error(utilities::generic_errc::buffer_overrun);
    return count;
}
} //namespace buffers
