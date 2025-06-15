/* This file is part of Nokia HEIF library
 *
 * Copyright (c) 2015-2025 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: heif@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */

#include "imagegrid.hpp"

#include <limits>

#include "bitstream.hpp"

void writeImageGrid(const ImageGrid& grid, BitStream& output)
{
    bool write32BitFields = false;
    output.write8Bits(0);  // version = 0
    if ((grid.outputWidth > std::numeric_limits<std::uint16_t>::max()) &&
        (grid.outputHeight > std::numeric_limits<std::uint16_t>::max()))
    {
        write32BitFields = true;
    }

    output.write8Bits(write32BitFields);  // flags

    output.write8Bits(grid.rowsMinusOne);
    output.write8Bits(grid.columnsMinusOne);

    if (write32BitFields)
    {
        output.write32Bits(grid.outputWidth);
        output.write32Bits(grid.outputHeight);
    }
    else
    {
        output.write16Bits(static_cast<std::uint16_t>(grid.outputWidth));
        output.write16Bits(static_cast<std::uint16_t>(grid.outputHeight));
    }
}

/* 为什么Grid图可以这样解析？
 * 因为HEIF标准明确定义了grid数据的二进制格式：
 * 1. 版本号（1字节）
 * 2. 标志位（1字节）
 *    最低位bit0表示尺寸位宽：0=16位，1=32位
 *    其他位保留，必须为0
 * 3. 网格参数（各1字节）
 *    rowsMinusOne：实际行数 = value + 1
 *    columsMinusOne：实际列数 = value + 1
 * 4. 输出尺寸（2/4字节）
 *    根据标志位选择16位或32位存储
 */
ImageGrid parseImageGrid(BitStream& input)
{
    ImageGrid grid;

    input.read8Bits();                             // discard version
    bool read32BitFields = input.read8Bits() & 1;  // flags

    grid.rowsMinusOne    = input.read8Bits();
    grid.columnsMinusOne = input.read8Bits();

    if (read32BitFields)
    {
        grid.outputWidth  = input.read32Bits();
        grid.outputHeight = input.read32Bits();
    }
    else
    {
        grid.outputWidth  = input.read16Bits();
        grid.outputHeight = input.read16Bits();
    }

    return grid;
}
