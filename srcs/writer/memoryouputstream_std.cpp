#include <fstream>

#include "OutputStreamInterface.h"
#include "customallocator.hpp"
#include "memoryoutputstream.hpp"


namespace HEIF
{
    void MemoryOutputStream::write(const void* aBuf, uint64_t aCount)
    {
        if (aCount == 0) return;
        
        const uint8_t* src = static_cast<const uint8_t*>(aBuf);
        const size_t newPosition = mPosition + static_cast<size_t>(aCount);
        
        // 确保缓冲区足够大
        if (newPosition > mBuffer.size()) {
            mBuffer.resize(newPosition);
        }
        
        // 复制数据
        std::copy(src, src + aCount, mBuffer.begin() + mPosition);
        mPosition = newPosition;
    }

    void MemoryOutputStream::seekp(std::uint64_t aPos)
    {
        if (aPos > mBuffer.size()) {
            // 扩展缓冲区并用0填充空隙
            mBuffer.resize(static_cast<size_t>(aPos), 0);
        }
        mPosition = static_cast<size_t>(aPos);
    }

    std::uint64_t MemoryOutputStream::tellp()
    {
        return static_cast<std::uint64_t>(mPosition);
    };

    void MemoryOutputStream::remove()
    {
        mBuffer.clear();
        mPosition = 0;
    };

    bool MemoryOutputStream::is_open()
    {
        return true;
    }

    OutputStreamInterface* ConstructMemoryStream()
    {
        OutputStreamInterface* aFile = new MemoryOutputStream();
        if (!static_cast<MemoryOutputStream*>(aFile)->is_open())
        {
            delete aFile;
            aFile = nullptr;
        }
        return aFile;
    }
}  // namespace HEIF
 