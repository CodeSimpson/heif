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

/** This file includes some examples about using the reader API and writer API (example7).
 *  Note:
 *  - Binary execution fails if proper .heic files are not located in the directory.
 *  - Input files are available from https://github.com/nokiatech/heif_conformance
 *  - Code for image decoding or rendering is not present here.
 *  - Most error checking and proper cleanup in error cases are omitted to keep examples brief. */

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <queue>

#include "heifreader.h"
#include "heifwriter.h"

using namespace std;
using namespace HEIF;

void example1();
void example2();
void example3();
void example4();
void example5();
void example6();
void example7();
void example8();
void example9();
void printInfo(const char* file);

/// Access and read a cover image
void example1()
{
    auto* reader = Reader::Create();
    // Input file available from https://github.com/nokiatech/heif_conformance
    const char* filename = "C003.heic";
    if (reader->initialize(filename) != ErrorCode::OK)
    {
        printInfo(filename);
        Reader::Destroy(reader);
        return;
    }

    FileInformation info;
    reader->getFileInformation(info);

    // Find the item ID
    ImageId itemId;
    reader->getPrimaryItem(itemId);

    uint64_t memoryBufferSize = 1024 * 1024;
    auto* memoryBuffer        = new uint8_t[memoryBufferSize];
    reader->getItemDataWithDecoderParameters(itemId, memoryBuffer, memoryBufferSize);

    // Feed 'data' to decoder and display the cover image...

    delete[] memoryBuffer;

    Reader::Destroy(reader);
}

/// Access and read image item and its thumbnail
void example2()
{
    auto* reader = Reader::Create();

    // Input file available from https://github.com/nokiatech/heif_conformance
    const char* filename = "C012.heic";
    if (reader->initialize(filename) != ErrorCode::OK)
    {
        printInfo(filename);
        Reader::Destroy(reader);
        return;
    }

    FileInformation info;
    if (reader->getFileInformation(info) != ErrorCode::OK)
    {
        // handle error here...
        return;
    }

    // Verify that the file has one or several images in the MetaBox
    if (!(info.features & FileFeatureEnum::HasSingleImage || info.features & FileFeatureEnum::HasImageCollection))
    {
        return;
    }

    // Find the item ID of the first master image
    Array<ImageId> itemIds;
    reader->getMasterImages(itemIds);
    if (itemIds.size == 0)
    {
        return;  // this should not happen with the example file
    }
    const ImageId masterId = itemIds[0];
    uint64_t itemSize      = 1024 * 1024;
    auto* itemData         = new uint8_t[itemSize];

    const auto metaBoxFeatures = info.rootMetaBoxInformation.features;  // For convenience
    if (metaBoxFeatures & MetaBoxFeatureEnum::HasThumbnails)
    {
        // Thumbnail references ('thmb') are from the thumbnail image to the master image
        reader->getReferencedToItemListByType(masterId, "thmb", itemIds);
        if (itemIds.size == 0)
        {
            return;  // this should not happen with the example file
        }
        const auto thumbnailId = itemIds[0];

        if (reader->getItemDataWithDecoderParameters(thumbnailId.get(), itemData, itemSize) == ErrorCode::OK)
        {
            // ...decode data and display the image, show master image later
        }
    }
    else
    {
        // There was no thumbnail, show just the master image
        if (reader->getItemDataWithDecoderParameters(masterId.get(), itemData, itemSize) == ErrorCode::OK)
        {
            // ...decode and display...
        }
    }

    Reader::Destroy(reader);
    delete[] itemData;
}

/// Access and read image items and their thumbnails in a collection
void example3()
{
    auto* reader = Reader::Create();
    typedef ImageId MasterItemId;
    typedef ImageId ThumbnailItemId;
    Array<ImageId> itemIds;
    map<MasterItemId, ThumbnailItemId> imageMap;

    // Input file available from https://github.com/nokiatech/heif_conformance
    const char* filename = "C012.heic";
    if (reader->initialize(filename) != ErrorCode::OK)
    {
        printInfo(filename);
        Reader::Destroy(reader);
        return;
    }

    FileInformation info;
    reader->getFileInformation(info);

    // Find item IDs of master images
    reader->getMasterImages(itemIds);

    // Find thumbnails for each master. There can be several thumbnails for one master image, get narrowest ones
    // here.
    for (const auto masterId : itemIds)
    {
        // Thumbnail references ('thmb') are from the thumbnail image to the master image
        Array<ImageId> thumbIds;
        reader->getReferencedToItemListByType(masterId, "thmb", thumbIds);

        const auto thumbId = std::min_element(thumbIds.begin(), thumbIds.end(),
                                              [&](ImageId a, ImageId b)
                                              {
                                                  uint32_t widthA, widthB;
                                                  reader->getWidth(a, widthA);
                                                  reader->getWidth(b, widthB);
                                                  return (widthA < widthB);
                                              });
        if (thumbId != thumbIds.end())  // For images without thumbnail thumbId equals to end()
        {
            imageMap[masterId] = thumbId->get();
        }
    }

    // We have now item IDs of thumbnails and master images. Decode and show them from imageMap as wanted.
    Reader::Destroy(reader);
}

/// Access and read derived images
void example4()
{
    auto* reader = Reader::Create();
    Array<ImageId> itemIds;

    // Input file available from https://github.com/nokiatech/heif_conformance
    const char* filename = "C008.heic";
    if (reader->initialize(filename) != ErrorCode::OK)
    {
        printInfo(filename);
        Reader::Destroy(reader);
        return;
    }

    FileInformation info;
    reader->getFileInformation(info);

    // Find item IDs of 'iden' (identity transformation) type derived images
    reader->getItemListByType("iden", itemIds);

    const auto itemId = itemIds[0];  // For demo purposes, assume there was one 'iden' item
    if (itemIds.size == 0)
    {
        return;  // this should not happen with the example file
    }

    // 'dimg' item reference points from the 'iden' derived item to the input(s) of the derivation
    Array<ImageId> referencedImages;
    reader->getReferencedFromItemListByType(itemId, "dimg", referencedImages);
    if (referencedImages.size == 0)
    {
        return;  // this should not happen with the example file
    }
    const ImageId sourceItemId = referencedImages[0];  // For demo purposes, assume there was one

    // Get 'iden' item properties to find out what kind of derivation it is
    Array<ItemPropertyInfo> propertyInfos;
    reader->getItemProperties(itemId, propertyInfos);

    unsigned int rotation = 0;
    for (const auto& property : propertyInfos)
    {
        // For example, handle 'irot' transformational property is anti-clockwise rotation
        if (property.type == ItemPropertyType::IROT)
        {
            // Get property struct by index to access rotation angle
            Rotate irot;
            reader->getProperty(property.index, irot);
            rotation = irot.angle;
            break;  // Assume only one property
        }
    }

    cout << "To render derived image item ID " << itemId.get() << ":" << endl;
    cout << "-retrieve data for source image item ID " << sourceItemId.get() << endl;
    cout << "-rotating image " << rotation << " degrees." << endl;

    Reader::Destroy(reader);
}

/// Access and read media track samples, thumbnail track samples and timestamps
void example5()
{
    auto* reader = Reader::Create();
    Array<uint32_t> itemIds;

    // Input file available from https://github.com/nokiatech/heif_conformance
    const char* filename = "C032.heic";
    if (reader->initialize(filename) != ErrorCode::OK)
    {
        printInfo(filename);
        Reader::Destroy(reader);
        return;
    }
    FileInformation info;
    reader->getFileInformation(info);

    // Print information for every track read
    for (const auto& trackProperties : info.trackInformation)
    {
        const auto sequenceId = trackProperties.trackId;
        cout << "Track ID " << sequenceId.get() << endl;  // Context ID corresponds to the track ID

        if (trackProperties.features & TrackFeatureEnum::IsMasterImageSequence)
        {
            cout << "This is a master image sequence." << endl;
        }

        if (trackProperties.features & TrackFeatureEnum::IsThumbnailImageSequence)
        {
            // Assume there is only one type track reference, so check reference type and master track ID(s) from
            // the first one.
            if (trackProperties.referenceTrackIds.size == 0)
            {
                return;  // this should not happen with the example file
            }
            const auto tref = trackProperties.referenceTrackIds[0];
            cout << "Track reference type is '" << tref.type.value << "'" << endl;
            cout << "This is a thumbnail track for track ID ";
            for (const auto masterTrackId : tref.trackIds)
            {
                cout << masterTrackId.get() << endl;
            }
        }

        Array<TimestampIDPair> timestamps;
        reader->getItemTimestamps(sequenceId, timestamps);
        cout << "Sample timestamps:" << endl;
        for (const auto& timestamp : timestamps)
        {
            cout << " Timestamp=" << timestamp.timeStamp << "ms, sample ID=" << timestamp.itemId.get() << endl;
        }

        for (const auto& sampleProperties : trackProperties.sampleProperties)
        {
            // A sample might have decoding dependencies. The simplest way to handle this is just to always ask and
            // decode all dependencies.
            Array<SequenceImageId> itemsToDecode;
            reader->getDecodeDependencies(sequenceId, sampleProperties.sampleId, itemsToDecode);
            for (auto dependencyId : itemsToDecode)
            {
                uint64_t size    = 1024 * 1024;
                auto* sampleData = new uint8_t[size];
                reader->getItemDataWithDecoderParameters(sequenceId, dependencyId, sampleData, size);

                // Feed data to decoder...

                delete[] sampleData;
            }
            // Store or show the image...
        }
    }

    Reader::Destroy(reader);
}

/// Access and read media alternative
void example6()
{
    auto* reader = Reader::Create();
    Array<uint32_t> itemIds;

    // Input file available from https://github.com/nokiatech/heif_conformance
    const char* filename = "C032.heic";
    if (reader->initialize(filename) != ErrorCode::OK)
    {
        printInfo(filename);
        Reader::Destroy(reader);
        return;
    }

    FileInformation info;
    reader->getFileInformation(info);

    SequenceId trackId = 0;

    if (info.trackInformation.size > 0)
    {
        const auto& trackInfo = info.trackInformation[0];
        trackId               = trackInfo.trackId;

        if (trackInfo.features & TrackFeatureEnum::HasAlternatives)
        {
            const SequenceId alternativeTrackId = trackInfo.alternateTrackIds[0];  // Take the first alternative
            uint32_t alternativeWidth;
            reader->getDisplayWidth(alternativeTrackId, alternativeWidth);
            uint32_t trackWidth;
            reader->getDisplayWidth(trackId, trackWidth);

            if (trackWidth > alternativeWidth)
            {
                cout << "The alternative track has wider display width, let's use it from now on..." << endl;
                trackId = alternativeTrackId;
            }
        }
    }
    Reader::Destroy(reader);
}

// Example about reading file and writing it to output. Note most error checking omitted.
void example7()
{
    // create reader and writer instances
    auto* reader = Reader::Create();
    auto* writer = Writer::Create();

    // partially configure writer output
    OutputConfig writerOutputConf{};
    writerOutputConf.fileName        = "example7_output.heic";
    writerOutputConf.progressiveFile = true;

    // Input file available from https://github.com/nokiatech/heif_conformance
    const char* filename = "C014.heic";
    if (reader->initialize(filename) != ErrorCode::OK)
    {
        printInfo(filename);
        Reader::Destroy(reader);
        Writer::Destroy(writer);
        return;
    }
    // read major brand of file and store it to writer config.
    FourCC inputMajorBrand{};
    reader->getMajorBrand(inputMajorBrand);

    // add major brand to writer config
    writerOutputConf.majorBrand = inputMajorBrand;

    // read compatible brands of file and store it to writer config.
    Array<FourCC> inputCompatibleBrands{};
    reader->getCompatibleBrands(inputCompatibleBrands);

    // add compatible brands to writer config
    writerOutputConf.compatibleBrands = inputCompatibleBrands;

    // initialize writer now that we have all the needed information from reader
    if (writer->initialize(writerOutputConf) != ErrorCode::OK)
    {
        return;
    }
    // get information about all input file content
    FileInformation fileInfo{};
    reader->getFileInformation(fileInfo);

    // map which input decoder config id matches the writer output decoder config ids
    map<DecoderConfigId, DecoderConfigId> inputToOutputDecoderConfigIds;

    // Image Rotation property as an example of Image Properties:
    map<PropertyId, Rotate> imageRotationProperties;

    // map which input image property maach the writer output property.
    map<PropertyId, PropertyId> inputToOutputImageProperties;

    // go through all items in input file and store master image decoder configs
    for (const auto& image : fileInfo.rootMetaBoxInformation.itemInformations)
    {
        if (image.features & ItemFeatureEnum::IsMasterImage)
        {
            // read image decoder config and store its id if not seen before
            DecoderConfiguration inputdecoderConfig{};
            reader->getDecoderParameterSets(image.itemId, inputdecoderConfig);
            if (!inputToOutputDecoderConfigIds.count(inputdecoderConfig.decoderConfigId))
            {
                // feed new decoder config to writer and store input to output id pairing information
                DecoderConfigId outputDecoderConfigId;
                writer->feedDecoderConfig(inputdecoderConfig.decoderSpecificInfo, outputDecoderConfigId);
                inputToOutputDecoderConfigIds[inputdecoderConfig.decoderConfigId] = outputDecoderConfigId;
            }

            // read image data
            Data imageData{};
            imageData.size = image.size;
            imageData.data = new uint8_t[imageData.size];
            reader->getItemData(image.itemId, imageData.data, imageData.size, false);

            // feed image data to writer
            imageData.mediaFormat     = MediaFormat::HEVC;
            imageData.decoderConfigId = inputToOutputDecoderConfigIds.at(inputdecoderConfig.decoderConfigId);
            MediaDataId outputMediaId;
            writer->feedMediaData(imageData, outputMediaId);
            delete[] imageData.data;

            // create new image based on that data:
            ImageId outputImageId;
            writer->addImage(outputMediaId, outputImageId);

            // if this input image was the primary image -> also mark output image as primary image
            if (image.features & ItemFeatureEnum::IsPrimaryImage)
            {
                writer->setPrimaryItem(outputImageId);
            }

            // copy other properties over
            Array<ItemPropertyInfo> imageProperties;
            reader->getItemProperties(image.itemId, imageProperties);

            for (const auto& imageProperty : imageProperties)
            {
                switch (imageProperty.type)
                {
                case ItemPropertyType::IROT:
                {
                    if (!imageRotationProperties.count(imageProperty.index))
                    {
                        // if we haven't yet read this property for other image -> do so and add it to writer as
                        // well
                        Rotate rotateInfo{};
                        reader->getProperty(imageProperty.index, rotateInfo);
                        imageRotationProperties[imageProperty.index] = rotateInfo;

                        // create new property for images in writer
                        writer->addProperty(rotateInfo, inputToOutputImageProperties[imageProperty.index]);
                    }
                    // associate property with this output image
                    writer->associateProperty(image.itemId, inputToOutputImageProperties.at(imageProperty.index),
                                              imageProperty.essential);
                    break;
                }
                //
                // ... other properties
                //
                default:
                    break;
                }
            }
        }
    }

    writer->finalize();

    Reader::Destroy(reader);
    Writer::Destroy(writer);
}

/// Read Exif metadata. Note that most error checking is omitted.
void example8()
{
    auto* reader = Reader::Create();

    // Try opening a file with an "Exif" item.
    // The file is available from https://github.com/nokiatech/heif_conformance
    const char* filename = "C034.heic";
    if (reader->initialize(filename) != ErrorCode::OK)
    {
        printInfo(filename);
        Reader::Destroy(reader);
        return;
    }

    FileInformation fileInfo{};
    reader->getFileInformation(fileInfo);

    // Find the primary item ID.
    ImageId primaryItemId;
    reader->getPrimaryItem(primaryItemId);

    // Find item(s) referencing to the primary item with "cdsc" (content describes) item reference.
    Array<ImageId> metadataIds;
    reader->getReferencedToItemListByType(primaryItemId, "cdsc", metadataIds);
    if (metadataIds.size == 0)
    {
        return;  // this should not happen with the example file
    }
    ImageId exifItemId = metadataIds[0];

    // Optional: verify the item ID we got is really of "Exif" type.
    FourCC itemType;
    reader->getItemType(exifItemId, itemType);
    if (itemType != "Exif")
    {
        return;
    }

    // Get item size from parsed information.
    const auto it = std::find_if(fileInfo.rootMetaBoxInformation.itemInformations.begin(),
                                 fileInfo.rootMetaBoxInformation.itemInformations.end(),
                                 [exifItemId](ItemInformation a) { return a.itemId == exifItemId; });
    if (it == fileInfo.rootMetaBoxInformation.itemInformations.end())
    {
        Reader::Destroy(reader);
        return;
    }
    auto itemSize = it->size;

    // Request item data.
    auto* memoryBuffer = new uint8_t[itemSize];
    reader->getItemData(metadataIds[0], memoryBuffer, itemSize);

    // Write Exif item data to a file.
    // Note this data does not contain Exif payload only. The payload is preceded by 4-byte exif_tiff_header_offset
    // field as defined by class ExifDataBlock() in ISO/IEC 23008-12:2017.
    ofstream file("exifdata.bin", ios::out | ios::binary);
    file.write(reinterpret_cast<char*>(memoryBuffer), static_cast<std::streamsize>(itemSize));
    delete[] memoryBuffer;

    Reader::Destroy(reader);
}

void convertAnnexBToLengthPrefix(std::vector<uint8_t>& buffer) {
    uint32_t bufferSize = buffer.size();
    for (uint32_t i = 0; i < bufferSize; ) {
        // 检测3/4字节起始码
        bool isStartCode = false;
        uint32_t startCodeLen = 0;
        
        if (i + 2 < bufferSize && buffer[i] == 0x00 && buffer[i+1] == 0x00 && buffer[i+2] == 0x01) {
            startCodeLen = 3;
            isStartCode = true;
        } else if (i + 3 < bufferSize && buffer[i] == 0x00 && buffer[i+1] == 0x00 && 
                   buffer[i+2] == 0x00 && buffer[i+3] == 0x01) {
            startCodeLen = 4;
            isStartCode = true;
        }

        if (!isStartCode) {
            i++;
            continue;
        }

        uint32_t nalStart = i;
        i += startCodeLen; // 跳过起始码
        if (i >= bufferSize) break;

        uint8_t nalType = (buffer[i] >> 1) & 0x3F;
        // i++; // 移动到NAL数据区

        // 查找下一个起始码
        uint32_t nalEnd = bufferSize;
        for (uint32_t j = i; j < bufferSize; ++j) {
            if (j + 2 < bufferSize && buffer[j] == 0x00 && buffer[j+1] == 0x00 && buffer[j+2] == 0x01) {
                nalEnd = j - startCodeLen + 1;
                break;
            } else if (j + 3 < bufferSize && buffer[j] == 0x00 && buffer[j+1] == 0x00 && 
                      buffer[j+2] == 0x00 && buffer[j+3] == 0x01) {
                nalEnd = j;
                break;
            }
        }
        // 提取NAL数据（不包含起始码）
        uint32_t nalSize = nalEnd - nalStart - startCodeLen;
        if (startCodeLen == 4) {
            buffer[i - startCodeLen] = ((nalSize >> 24) & 0xFF);
            buffer[i - startCodeLen + 1] = ((nalSize >> 16) & 0xFF);
            buffer[i - startCodeLen + 2] = ((nalSize >> 8) & 0xFF);
            buffer[i - startCodeLen + 3] = (nalSize & 0xFF);
        } else if (startCodeLen == 3) {
            buffer[i - startCodeLen] = ((nalSize >> 16) & 0xFF);
            buffer[i - startCodeLen + 1] = ((nalSize >> 8) & 0xFF);
            buffer[i - startCodeLen + 2] = (nalSize & 0xFF);
        } else {
            cout << "error: startCodeLen = " << startCodeLen << endl;
        }

        i = nalEnd; // 跳转到下一个单元
    }
}

Array<DecoderSpecificInfo> getCsdData(const uint8_t* buffer, uint32_t bufferSize, uint32_t& csdSize) {
    uint32_t spsSize = 0;
    uint32_t ppsSize = 0;
    uint32_t vpsSize = 0;

    Array<DecoderSpecificInfo> csdData(3);
    for (uint32_t i = 0; i < bufferSize; ++i) {
        // 查找起始码 0x000001
        if (i + 3 < bufferSize && buffer[i] == 0x00 && buffer[i + 1] == 0x00 && buffer[i + 2] == 0x00 && buffer[i + 3] == 0x01) {
            uint8_t nalType = (buffer[i + 4] >> 1) & 0x3F;
            if (nalType == 32) { // VPS
                // vpsSize = i;
                for (uint32_t j = i + 4; j < bufferSize; ++j) {
                    if (j + 3 < bufferSize && buffer[j] == 0x00 && buffer[j + 1] == 0x00 && buffer[j + 2] == 0x00 && buffer[j + 3] == 0x01) {
                        vpsSize = j - i;
                        csdData[0].decSpecInfoType = DecoderSpecInfoType::HEVC_VPS;
                        csdData[0].decSpecInfoData = Array<uint8_t>(buffer + i, buffer + j);
                        break;
                    }
                }
            } else if (nalType == 33) { // SPS
                // spsSize = i;
                for (uint32_t j = i + 4; j < bufferSize; ++j) {
                    if (j + 3 < bufferSize && buffer[j] == 0x00 && buffer[j + 1] == 0x00 && buffer[j + 2] == 0x00 && buffer[j + 3] == 0x01) {
                        spsSize = j - i;
                        csdData[1].decSpecInfoType = DecoderSpecInfoType::HEVC_SPS;
                        csdData[1].decSpecInfoData = Array<uint8_t>(buffer + i, buffer + j);
                        break;
                    }
                }
            } else if (nalType == 34) { // PPS
                // ppsSize = i;
                for (uint32_t j = i + 4; j < bufferSize; ++j) {
                    if (j + 3 < bufferSize && buffer[j] == 0x00 && buffer[j + 1] == 0x00 && buffer[j + 2] == 0x00 && buffer[j + 3] == 0x01) {
                        ppsSize = j - i;
                        csdData[2].decSpecInfoType = DecoderSpecInfoType::HEVC_PPS;
                        csdData[2].decSpecInfoData = Array<uint8_t>(buffer + i, buffer + j);
                        break;
                    }
                }
            }
        }
    }
    cout << "vpsSize:" << csdData[0].decSpecInfoData.size << " spsSize:" << csdData[1].decSpecInfoData.size << " ppsSize:" << csdData[2].decSpecInfoData.size << endl;
    csdSize = vpsSize + spsSize + ppsSize;
    return csdData;
}

void example9()
{
    std::queue<std::vector<uint8_t>> primaryHeicBuffer;
    std::string basePath = "./heicenc/primaryBitstream_";
    uint32_t count = 0;
    while (true) {
        // 生成文件名
        std::string filePath = basePath + std::to_string(count) + ".h265";
        // 以二进制模式打开文件
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            break; // 文件不存在时退出循环
        }
        // 获取文件大小并调整vector容量
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> buffer(size);
        // 读取数据到vector
        if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            primaryHeicBuffer.push(std::move(buffer));
        } else {
            std::cout << "error: only" << file.gcount() << " could be read" << std::endl;
            break;
        }
        count++;
    }

    std::queue<std::vector<uint8_t>> gainmapHeicBuffer;
    basePath = "./heicenc/gainmapBitstream_";
    count = 0;
    while(true) {
        // 生成文件名
        std::string filePath = basePath + std::to_string(count) + ".h265";
        // 以二进制模式打开文件
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            break; // 文件不存在时退出循环
        }
        // 获取文件大小并调整vector容量
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> buffer(size);
        // 读取数据到vector
        if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            gainmapHeicBuffer.push(std::move(buffer));
        } else {
            std::cout << "error: only" << file.gcount() << " could be read" << std::endl;
            break;
        }
        count++;
    }

    std::vector<uint8_t> isoMetadataBitstream;
    std::string filePath = "./heicenc/isoMetadataBitstream.bin";
    // 以二进制模式打开文件
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (file.is_open()) {
        // 获取文件大小并调整vector容量
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        isoMetadataBitstream.resize(size + 1);
        if (file.read(reinterpret_cast<char*>(isoMetadataBitstream.data()) + 1, size)) {
            std::cout << "all characters from isoMetadataBitstream read successfully." << std::endl;
        } else {
            std::cout << "error: only" << file.gcount() << " could be read" << std::endl;
        }
    }

    std::cout << "primaryHeicBuffer size:" << primaryHeicBuffer.size() << " gainmapHeicBuffer size:"<< gainmapHeicBuffer.size()
              << " isoMetadataBitstream size:" << isoMetadataBitstream.size() << std::endl;

    // 初始化主图参数
    const uint32_t tileWidth = 512;
    const uint32_t tileHeight = 512;
    const uint32_t fullWidth = 3072;
    const uint32_t fullHeight = 4096;
    // 初始化gainmap图参数
    const uint32_t gainmapFullWidth = 1536;
    const uint32_t gainmapFullHeight = 2048;

    // 创建Writer实例
    Writer* writer = Writer::Create();
    OutputConfig writerOutputConf{};
    writerOutputConf.fileName = "IMG_20250725_173515_Nokia.HEIC";
    writerOutputConf.progressiveFile = true;
    writerOutputConf.majorBrand = "heic";
    writerOutputConf.compatibleBrands = Array<FourCC>(3);
    writerOutputConf.compatibleBrands[0] = FourCC("mif1");
    writerOutputConf.compatibleBrands[1] = FourCC("heic");
    writerOutputConf.compatibleBrands[2] = FourCC("tmap");

    // 初始化Writer
    if(writer->initialize(writerOutputConf) != ErrorCode::OK) {
        cout << "initialize failed!" << endl;
        Writer::Destroy(writer);
        return;
    }

    // ============================================== primary image ==============================================
    // 准备HEVC解码配置
    uint32_t csdDataSize = 0;
    Array<DecoderSpecificInfo> decoderConfig = getCsdData(primaryHeicBuffer.front().data(), primaryHeicBuffer.front().size(), csdDataSize);
    cout << "read csd data size:" << csdDataSize << " decoderConfig size:" << decoderConfig.size << endl;
    DecoderConfigId decoderConfigId;

    if(writer->feedDecoderConfig(decoderConfig, decoderConfigId) != ErrorCode::OK) {
        cout << "feedDecoderConfig failed!" << endl;
        return;
    }

    // 存储所有分块ImageID
    std::vector<ImageId> tileIds;
    const uint32_t cols = fullWidth / tileWidth;  // 6列
    const uint32_t rows = fullHeight / tileHeight; // 8行

    // 处理所有分块
    while(!primaryHeicBuffer.empty())
    {
        auto& tileData = primaryHeicBuffer.front();
        cout << "primage iamge gridSize:" << tileData.size() << endl;
        convertAnnexBToLengthPrefix(tileData);
        // 封装分块数据
        Data mediaData;
        mediaData.mediaFormat = MediaFormat::HEVC;
        mediaData.data = tileData.data();
        mediaData.size = static_cast<uint32_t>(tileData.size());
        mediaData.decoderConfigId = decoderConfigId;

        MediaDataId mediaDataId;
        ErrorCode ret = writer->feedMediaData(mediaData, mediaDataId);
        if(ret != ErrorCode::OK) {
            cout << "feedMediaData failed! ret:" << static_cast<int>(ret) << endl;
            return;
        }

        // 添加为图像项
        ImageId tileId;
        if(writer->addImage(mediaDataId, tileId) != ErrorCode::OK) {
            cout << "addImage failed!" << endl;
            return;
        }
        if (writer->setImageHidden(tileId, true) != ErrorCode::OK) {
            cout << "setImageHidden failed!" << endl;
            return;
        }

        cout << "mediaDataId:" <<  mediaDataId.get() << " tileId:" << tileId.get() << endl;
        tileIds.push_back(tileId);
        primaryHeicBuffer.pop();
    }

    // 创建网格组合
    Grid grid;
    grid.columns = cols;          // 6列
    grid.rows = rows;             // 8行
    grid.outputWidth = fullWidth; // 总宽度3072
    grid.outputHeight = fullHeight;// 总高度4096
    cout << "gridrows:" << grid.rows << " gridCols:" << grid.columns << endl;
    grid.imageIds = Array<ImageId>(tileIds.size());
    for (uint32_t i = 0; i < tileIds.size(); ++i)
    {
        grid.imageIds[i] = tileIds[i];
    }

    // 添加网格派生图像
    ImageId gridId;
    if(writer->addDerivedImageItem(grid, gridId) != ErrorCode::OK) {
        cout << "addDerivedImageItem failed!" << endl;
        return;
    }

    // 设置主图像
    if(writer->setPrimaryItem(gridId) != ErrorCode::OK) {
        cout << "setPrimaryItem failed!" << endl;
        return;
    }

    // add pixi property
    PixelInformation pixi;
    Array<uint8_t> bitsPerChannel(3);
    bitsPerChannel[0] = 8;
    bitsPerChannel[1] = 8;
    bitsPerChannel[2] = 8;
    pixi.bitsPerChannel = bitsPerChannel;
    PropertyId pixiPropertyId = 0;
    writer->addProperty(pixi, pixiPropertyId);
    writer->associateProperty(gridId, pixiPropertyId, true);

    // add colr property
    ColourInformation colr;
    colr.colourType = "nclx";
    colr.colourPrimaries = 12;
    colr.transferCharacteristics = 13;
    colr.matrixCoefficients = 2;
    colr.fullRangeFlag = 1;
    PropertyId colrPropertyId = 0;
    writer->addProperty(colr, colrPropertyId);
    writer->associateProperty(gridId, colrPropertyId, true);


    // =========================================== gainmap image ===============================================
    uint32_t gainmapCsdDataSize = 0;
    Array<DecoderSpecificInfo> gainmapDecoderConfig = getCsdData(gainmapHeicBuffer.front().data(), gainmapHeicBuffer.front().size(), gainmapCsdDataSize);
    cout << "read csd data size:" << gainmapCsdDataSize << " decoderConfig size:" << gainmapDecoderConfig.size << endl;
    DecoderConfigId gainmapDecoderConfigId;

    if(writer->feedDecoderConfig(gainmapDecoderConfig, gainmapDecoderConfigId) != ErrorCode::OK) {
        cout << "feedGainmapDecoderConfig failed!" << endl;
        return;
    }

    // 存储所有分块ImageID
    std::vector<ImageId> gainmapTileIds;
    const uint32_t gainmapCols = gainmapFullWidth / tileWidth;  // 3列
    const uint32_t gainmapRows = gainmapFullHeight / tileHeight; // 4行

    // 处理所有分块
    while(!gainmapHeicBuffer.empty())
    {
        auto& tileData = gainmapHeicBuffer.front();
        cout << "gainmap iamge gridSize:" << tileData.size() << endl;
        convertAnnexBToLengthPrefix(tileData);
        // 封装分块数据
        Data mediaData;
        mediaData.mediaFormat = MediaFormat::HEVC;
        mediaData.data = tileData.data();
        mediaData.size = static_cast<uint32_t>(tileData.size());
        mediaData.decoderConfigId = gainmapDecoderConfigId;

        MediaDataId mediaDataId;
        ErrorCode ret = writer->feedMediaData(mediaData, mediaDataId);
        if(ret != ErrorCode::OK) {
            cout << "feedMediaData failed! ret:" << static_cast<int>(ret) << endl;
            return;
        }

        // 添加为图像项
        ImageId tileId;
        if(writer->addImage(mediaDataId, tileId) != ErrorCode::OK) {
            cout << "addImage failed!" << endl;
            return;
        }
        if (writer->setImageHidden(tileId, true) != ErrorCode::OK) {
            cout << "setImageHidden failed!" << endl;
            return;
        }

        cout << "gainmapMediaDataId:" <<  mediaDataId.get() << " tileId:" << tileId.get() << endl;
        gainmapTileIds.push_back(tileId);
        gainmapHeicBuffer.pop();
    }

    // 创建网格组合
    Grid gainmapGrid;
    gainmapGrid.columns = gainmapCols;          // 3列
    gainmapGrid.rows = gainmapRows;             // 4行
    gainmapGrid.outputWidth = gainmapFullWidth; // 总宽度1536
    gainmapGrid.outputHeight = gainmapFullHeight;// 总高度2048
    cout << "gridrows:" << gainmapGrid.rows << " gridCols:" << gainmapGrid.columns << endl;
    gainmapGrid.imageIds = Array<ImageId>(gainmapTileIds.size());
    for (uint32_t i = 0; i < gainmapTileIds.size(); ++i)
    {
        gainmapGrid.imageIds[i] = gainmapTileIds[i];
    }

    // 添加网格派生图像
    ImageId gainmapGridId;
    if(writer->addDerivedImageItem(gainmapGrid, gainmapGridId) != ErrorCode::OK) {
        cout << "addDerivedImageItem failed!" << endl;
        return;
    }

    // add pixi property
    PixelInformation gainmapPixi;
    Array<uint8_t> gainmapbBitsPerChannel(1);
    gainmapbBitsPerChannel[0] = 8;
    // gainmapbBitsPerChannel[1] = 8;
    // gainmapbBitsPerChannel[2] = 8;
    gainmapPixi.bitsPerChannel = gainmapbBitsPerChannel;
    PropertyId gainmapPixiPropertyId = 0;
    writer->addProperty(gainmapPixi, gainmapPixiPropertyId);
    writer->associateProperty(gainmapGridId, gainmapPixiPropertyId, true);

    // add colr property
    ColourInformation gainmapColr;
    gainmapColr.colourType = "nclx";
    gainmapColr.colourPrimaries = 2;
    gainmapColr.transferCharacteristics = 2;
    gainmapColr.matrixCoefficients = 2;
    gainmapColr.fullRangeFlag = 1;
    PropertyId gainmapColrPropertyId = 0;
    writer->addProperty(gainmapColr, gainmapColrPropertyId);
    writer->associateProperty(gainmapGridId, gainmapColrPropertyId, true);

    // ===================================== tmap ==========================================
    if (!isoMetadataBitstream.empty()) {
        cout << "tmap size:" << isoMetadataBitstream.size() << endl;

        // 封装tmap数据
        Data mediaData;
        mediaData.mediaFormat = MediaFormat::TMAP;
        mediaData.data = isoMetadataBitstream.data();
        mediaData.size = isoMetadataBitstream.size();
        mediaData.decoderConfigId = gainmapDecoderConfigId;

        MediaDataId mediaDataId;
        ErrorCode ret = writer->feedMediaData(mediaData, mediaDataId);
        if(ret != ErrorCode::OK) {
            cout << "feedMediaData failed! ret:" << static_cast<int>(ret) << endl;
            return;
        }

        // create new image based on that data:
        ImageId metaImageId;
        writer->addImage(mediaDataId, metaImageId);

        // add pixi property
        PixelInformation tmapPixi;
        Array<uint8_t> bitsPerChannel(3);
        bitsPerChannel[0] = 8;
        bitsPerChannel[1] = 8;
        bitsPerChannel[2] = 8;
        tmapPixi.bitsPerChannel = bitsPerChannel;
        PropertyId tmapPixiPropertyId = 0;
        writer->addProperty(tmapPixi, tmapPixiPropertyId);
        writer->associateProperty(metaImageId, tmapPixiPropertyId, true);

        // add colr property
        ColourInformation tmapColr;
        tmapColr.colourType = "nclx";
        tmapColr.colourPrimaries = 12;
        tmapColr.transferCharacteristics = 13;
        tmapColr.matrixCoefficients = 2;
        tmapColr.fullRangeFlag = 1;
        PropertyId tmapColrPropertyId = 0;
        writer->addProperty(tmapColr, tmapColrPropertyId);
        writer->associateProperty(metaImageId, tmapColrPropertyId, true);

        // 添加对主图和gainmap图的引用
        Array<ImageId> toImageIds(2);
        toImageIds[0] = gridId;
        toImageIds[1] = gainmapGridId;
        writer->addDimgItemReference(metaImageId, toImageIds);

        // add alternaltive entity
        GroupId altrId;
        writer->createAlternativesGroup(altrId);
        writer->addToGroup(altrId, metaImageId);
        writer->addToGroup(altrId, gridId);
    }

    // 最终封装
    if(writer->finalize() != ErrorCode::OK) {
        cout << "finalize failed!" << endl;
        return;
    }

    // 清理资源
    Writer::Destroy(writer);
}

void printInfo(const char* filename)
{
    cout << "Can't find input file: " << filename << ". "
         << "Please download it from https://github.com/nokiatech/heif_conformance "
         << "and place it in same directory with the executable." << endl;
}

int main()
{
    // example1();
    // example2();
    // example3();
    // example4();
    // example5();
    // example6();
    // example7();
    // example8();
    example9();
}
