#ifndef MEMORYOUTPUTSTREAM_HPP
#define MEMORYOUTPUTSTREAM_HPP

#include <vector>
#include "OutputStreamInterface.h"
#include "customallocator.hpp"

namespace HEIF
{
    class MemoryOutputStream : public OutputStreamInterface
    {
    public:
        MemoryOutputStream(size_t initialCapacity = 0)
            : mPosition(0)
        {
            mBuffer.reserve(initialCapacity);
        }

        ~MemoryOutputStream() override
        {
        }

        bool is_open();

        void write(const void* buf, uint64_t count) override;

        void seekp(std::uint64_t aPos) override;

        std::uint64_t tellp() override;

        void remove() override;

        // 获取底层内存指针
        const uint8_t* data() { return mBuffer.data(); }

        std::uint64_t size() { return mBuffer.size(); }

    private:
        std::vector<uint8_t> mBuffer; // 内存存储核心
        size_t mPosition;             // 当前写入位置
    };
 }  // namespace HEIF
 
 #endif
 