

## HEIF Library

### 1. 代码分析

* HEIF Reader API and Library (under srcs/api/reader/)

  Reader为纯虚类，确定API接口，具体实现在srcs/reader/heifreaderimpl.hpp中，

* HEIF文件基于ISO/IEC 23008-12标准，采用ISOBMFF作为容器结构。

* `meta` box解析函数分析

  **HeifReaderImpl::handleMeta(StreamIO& io) **

  ```c++
  HeifReaderImpl::handleMeta()
  --->HeifReaderImpl::readBox()		// get bitstream
  --->MetaBox::parseBox()				// get metaBox
  --->HeifReaderImpl::extractMetaBoxProperties()	// return MetaBoxProperties
      --->HeifReaderImpl::extractMetaBoxItemPropertiesMap()	// return ItemFeaturesMap，从HEIF的metaBox中提取所有项item和元数据项的特征属性，构建ItemId-->特征标记 映射表
      	--->doReferencesFromItemIdExist()		// 检查是否存在类型为xxx的并从当前item出发的主动引用
      	--->ItemFeature::setFeature()
      	...
      	--->doReferencesToItemIdExist()			// 检查当前item是否被类型为xxx item的引用
      	--->ItemFeature::setFeature()
      --->HeifReaderImpl::extractMetaBoxEntityToGroupMaps()
      	--->MetaBox::getGroupsListBox()::getEntityToGroupsBoxes()	// 提取实体（Entity）到分组（Group）的映射关系，一张图片就是一个实体
      --->HeifReaderImpl::extractMetaBoxFeatures()	// 返回描述整个MetaBox特性的MetaBoxFeature对象
  --->HeifReaderImpl::extractItems()	// return MetaBoxInfo
     	--->loadItemData()				// 加载Item对应的数据到bitsteam结构体中
      --->type == "grid"				// 解析网格图参数
      	parseImageGrid()
      	getReferencedFromItemListByType()	// 获得dimg((Drived Image Reference)引用类型指向的基础图像的itemIds
      --->type == "iovl"				// 解析覆盖图参数
      	parseImageOverlay()
      	getReferencedFromItemListByType()
      --->processItemProperties()		// 解析文件中独立项目和逻辑分组各自的属性信息
      	--->iprp.getItemProperties(itemId)
      	--->iprp.getItemProperties(groupId)
      --->extractItemInfoMap()		// 遍历HEIF文件中的每个item，构建itemId -> ItemInfo映射表
  --->HeifReaderImpl::processDecoderConfigProperties() // 提取并关联HEIF文件中图像项目的解码配置参数，核心目标是为每个图像项目（如HEVC/AVC/JPEG编码）绑定对应的解码器参数集（如HVCC/AVCC/JPEG配置）
  --->HeifReaderImpl::getMasterImages()	// 查找master image数量
  ```
  
  meta相关数据结构
  
  ```c++
  struct MetaBoxProperties {
  	MetaBoxFeature {
  		MetaBoxFeatureSet == Set<MetaBoxFeatureEnum::Feature> // enum Feature{IsSingleImage,IsImageCollection,HasMasterImages...}
  	}
  	ItemFeaturesMap == Map<ImageId, ItemFeature>
          ItemFeature {
          	ItemFeatureSet == Set<ItemFeatureEnum::Feature>	// enum Feature{IsMasterImage,IsThumbnailImage,IsAuxiliaryImage...}
      	}
  	Groupings == Vector<EntityGrouping>
          EntityGrouping {
          	FourCC == char value[5]	// Grouping type，分组类型，通过FourCC编码
              GroupId					// Grouping ID，分组的唯一标识符
              Array<uint32_t>			// Grouped entity IDs，属于该分组的实体ID列表，如图像项或元数据项的ID，entityId具有唯一性
      	}
  }
  ```
  
  ```c++
  struct MetaBoxInfo {
  	unsigned int displayableMasterImages
      ItemInfoMap == Map<ImageId, ItemInfo>
          ItemInfo {
          	ForCCInt type			// 项目类型编码，图像类：hvc1、avc1、grid、jpeg，元数据类：exif、xml、mime(通过MIME类型封装)
              String name				// item可读名称，可选，图像item：MasterImage、Thumbnail_200x200，元数据item：Exif_Data_1
              String contentype		// 描述内容的MIME类型，当type=mime时有效，图像：image/jpeg，元数据：application/rdf+xml
              String contenEncoding	// 指示内容的压缩或编码方式，可选
              uint32_t width
              uint32_t height
              uint64_t displayTime	// 图像的显示时间戳，用于序列图像的播放时序控制，为0时标识第一帧立即显示
      	}
      Map<ImageId, Grid>						// 保存itemId与其对应的网格图
          Grid {
          	uint32_t outputWidth			// 网格整体输出的宽度
              uint32_t outputHeight			// 网格整体输出的高度
              uint32_t columns				// 网格行数，存储时减1，使用时加1
              uint32_t rows					// 网格列数，存储时减1，使用时加1
              Array<ImageId> imageIds			// 网格图引用的图像ID
      	}
      Map<ImageId, Overlay>					// 保存itemId与其对对应的覆盖图
          Overlay {
          	uint16_t r						// 画布填充色
              uint16_t g
              uint16_t b
              uint16_t a
              uint32_t outputWidth			// 画布宽度
              init32_t outputHeight			// 画布高度
              Array<offset> offset
                  offset {
                  	int32_t horizontal		// 子图像水平偏移
                      int32_t vertical		// 子图像垂直偏移
              	}
              Array<ImageId> imageIds			// 覆盖图对应的的图像ID
      	}
      Properties == Map<uint32_t, PropertyTypeVector>
          PropertyTypeVector == Vector<ItemPropertyInfo>
          	ItemPropertyInfo {
          		ItemPropertyType			// 属性类型标识，对应HEIF规范中定义的4字符码ispe/irot/pixi等
                  PropertyId == uint32_t		// 属性数据定位索引，HEIF中属性的原始数据集中存储在ItemPropertyContainer中，通过index可以检索
                  bool essential				// 属性必要性标记，若为true，解析器必需解析此属性，否则可能导致渲染错误
      		}
  }
  ```
  
  > 为什么MetaBoxInfo::ItemInfoMap::ItemInfo中，type=mime时，contentType必须指定具体类型？
  >
  > ：这是由HEIF标准的设计机制和数据类型兼容性需求决定的。mime类型的定位为通用容器，用于HEIF中封装非原生支持格式活扩展格式，允许HEIF文件携带任意类型的数据（如PNG、JPEG、文本等），而不限于HEVC/AVC等原生支持的编码格式。



### 2. Box类型及作用

HEIF文件结构层级：

```c++
File
├── ftyp (File Type Box)
├── meta (MetaBox, 元数据容器) ← 我们讨论的上下文
│   ├── iinf (ItemInfoBox)       // 存储项目信息
│   ├── iloc (ItemLocationBox)   // 存储项目数据位置
│   ├── iprp (ItemPropertiesBox) // 项目属性
│   └── idat (ItemDataBox)       // 实际项目数据
└── mdat (Media Data Box)        // 可能的外部媒体数据
```



#### 2.1 ftyp box

HEIF文件遵循ISOBMFF标准，其文件结构由多个box组成。其中`ftyp` box为文件开头的关键元数据，用于声明文件的兼容标准和版本。以一张常见的heif图片`ftyp`box为例：

| Property name     | Property value         |
| ----------------- | ---------------------- |
| type              | ftyp                   |
| box_name          | FileTypeBox            |
| size              | 32                     |
| start             | 0                      |
| major_brand       | mif1                   |
| minor_version     | 0                      |
| compatible_brands | heic, mif1, miaf, MiHB |

**关键术语：**

* major_brand：主品牌，标识文件的核心格式标准，`mif1`是HEIF标准中的MIAF（Multi-Image Application Format），是HEIF的一个子集规范，专门定义多图像容器的通用处理规则，对应ISO/IEC 23000-22标准。

* minor_version：次版本，表示文件核心格式标准的版本号，为0表示基础版本。此处说明该文件遵循`mif1`主品牌的初时版本规范，未其用后续扩展功能。

* compatible_brand：兼容品牌，声明文件兼容的其他标准，解析器会按顺序尝试匹配支持的品牌。

  | 品牌标识 | 技术含义                                         | 应用场景         |
  | -------- | ------------------------------------------------ | ---------------- |
  | heic     | HEIF基础图像容器品牌，标识使用HEVC编码的单帧图像 | 普通HEIC照片     |
  | mif1     | 与major_brand重复                                | 多图像容器       |
  | miaf     | MIAF通用兼容品牌，表示符合MIAF规范的最小功能集   | 通用多图像解析   |
  | MiHB     | Apple扩展品牌，表示支持HDR图像                   | apple proraw照片 |

**技术验证：**

可以使用`mp4dump`工具验证，读取 `ftyp`Box

```bash
mp4dump *.heic | grep -A 3 "ftyp"
```

#### 2.2 etyp box

`etyp` box：声明文件可能依赖或关联的外部数据源类型，主要用于标识文件的扩展兼容性或外部引用关系。

​	在HEIF标准中，主文件通常应包含完整数据，但在特定设计场景中，确实可能出现HEIF文件需要依赖外部数据的情况。例如：

**分片存储（Fragmented Storage）**

- **场景** ：
  大型HEIF文件（如动态图像序列）被拆分为多个物理文件（分片），主文件仅包含元数据，实际图像数据存储在外部文件中。
- 示例 ：
  - 主文件：`photo.heic`（包含`etyp`声明兼容分片类型 `hvc1`）
  - 分片文件：`photo_data1.hvc`、`photo_data2.hvc`（存储HEVC编码的图片数据）
- **作用** ：
  `etyp`向解析器表明：“我的图像数据分散在外部`.hvc`文件中，请按HEVC标准加载它们”。

以iphone的livehphoto设计为例，主文件为静态HEIC图片，包含`etyp`声明`avc1`，外部数据为动态视频部分，独立存储在`.mov`文件中，相册应用通过`etyp`发现需要加载外部视频，实现“live”动态效果。这样做的好处在于可以减少主文件体积，外部数据可独立更新，敏感数据可以存储在外部加密文件中，通过`etyp`声明解密方式。

#### 2.3 meta box

`meta`box：元数据容器，是核心信息枢纽，负责组织和管理图像的所有元数据与结构信息。在HEIF的 **`ItemInfoEntry`** 结构中（ISO/IEC 23008-12 第8.11.6节），定义了`flag`字段，`flags` 字段为32位无符号整数，用于描述图像项的属性。`flag`最低位（Bit 0） 的官方定义为：**`hidden_item`** （隐藏项）：若为 `1`，表示该项**不应被默认渲染或展示** ，除非明确请求。

<img src="./pic/item_flag.png" style="zoom: 67%;" />

**核心作用：**

* 存储图像项的元数据

  - **图像项定义** ：描述文件内所有图像项（`item`）的属性，如主图像、缩略图、Alpha通道、深度图等。

  - **关联关系** ：定义图像项之间的引用关系（如主图与缩略图的关联、多帧图像的序列关系）。

* 编码参数与属性
  - 存储图像的解码配置（如HEVC的`hvcC`配置）、旋转角度、色彩空间等参数。

* 扩展元数据，当某个item被其他项通过cdsc引用时，标记此项为`IsMetadataItem`。
  - 包含Exif、XMP等辅助数据，用于存储拍摄信息、版权声明等。
* 资源定位
  - 通过`iloc` Box记录每个图像项在文件中的物理位置和大小，实现快速数据访问。

##### 2.3.1 关键子box

`meta`box通过嵌套子box实现具体功能：

| 子Box类型                     | 作用描述                                                     |
| ----------------------------- | ------------------------------------------------------------ |
| **`hdlr(HandlerBox)`**        | 声明元数据用途（如 `pict` 表示图像元数据，`Exif` 表示Exif数据容器）。 |
| **`pitm(PrimaryItemBox)`**    | **主图像项标识** ：指定默认显示的图像项ID（如主图的`item_ID`）。 |
| **`iloc(ItemLocationBox)`**   | **数据位置索引** ：记录每个图像项在文件中的存储偏移量、长度及编码方式。 |
| **`iinf(ItemInfoBox)`**       | **图像项信息表** ：列出所有图像项的类型（如`hvc1`/`av01`）、属性及基础描述。 |
| **`iref(ItemReferenceBox)`**  | **项引用关系** ：定义图像项间的依赖（如缩略图引用主图、Alpha通道关联主图）。包含不同的引用类型，如cdsc，表示内容描述引用，auxl表示辅助图像引用。 |
| **`iprp(ItemPropertiesBox)`** | **属性容器** ：包含图像项的编码参数（如HEVC的`hvcC`）、旋转、色彩属性等。 |
| **`Exif`**                    | 嵌套存储Exif元数据（需通过`hdlr`声明类型为`Exif`）。         |
| **`grpl(GroupsListBox)`**     | 用于定义文件中的分组逻辑。                                   |

**结构示例：**

```
meta
├── hdlr (handler_type='pict')   // 声明为图像元数据
├── pitm (item_ID=1)             // 主图像项为ID=1
├── iinf
│   ├── infe (item_ID=1, type='hvc1')  // 主图为HEVC编码
│   └── infe (item_ID=2, type='Exif')  // Exif数据项
├── iloc
│   ├── item 1: offset=0x500, length=1200  // 主图数据位置
│   └── item 2: offset=0xA00, length=300   // Exif数据位置
└── iprp
    └── hvcC (HEVC配置参数)       // 主图的解码配置
```

##### 2.3.2  grpl(GroupsListBox)

根据HEIF（ISO/IEC 23008-12）标准，`GroupsListBox`（FourCC为`grpl`）是`MetaBox`的子Box之一，用于定义文件中的分组逻辑。

**`GroupsListBox`的作用** ：

* **管理分组关系** ：它存储了多个`EntityToGroupBox`条目，每个条目描述一个“组”（Group）及其关联的实体（Entities）。

* 分组类型：

  * **`altr`**，Alternate Group，表示同一内容的不同版本，如不同分辨率或编码格式的图片。

  * **`eqiv`**，Equivalence Group，表示逻辑上等效的项，如不同语言的字幕。

  * **`ster`**，Stereo Group，专用于立体图像Stereo Image的关联管理，ster分组的核心目的是将多个实体Entities组织成一个立体图像。一个典型的`ster`分组在HEIF文件中的逻辑结构如下

    ```shell
    GroupsListBox（grpl）
    └── EntityToGroupBox（Stereo类型）
        ├── group_id: 100          // 分组唯一标识
        ├── type: 'ster'           // 分组类型为立体
        └── entity_ids: [1, 2]     // 实体ID列表（左眼=1，右眼=2）
    ```

**Entity与Group的关系：**

- 每个实体（如一张图片）可以通过`entityId`唯一标识。
- 分组允许将多个实体组织为逻辑单元（例如：主图+缩略图+深度图）。

假设HEIF文件中包含一张图片的三种分辨率版本（高、中、低），它们的`entityId`分别为1、2、3。通过`GroupsListBox`可以定义一个`altr`类型的分组：

```shell
GroupId: 100
Type: 'altr'
EntityIds: [1, 2, 3]
```

##### 2.3.3 iinf(ItemInfoBox)

​	`ItemInfoBox`用于存储项目信息，`grid`和`iovl`属于`iinf`的两种项目类型，其实际二进制数据在`idat(ItemDataBox)`或外部`mdat`中。

* 关键字box功能：

| Box 类型         | 四字符码 | 功能说明         | 与 grid/iovl 的关系                      |
| :--------------- | :------- | :--------------- | :--------------------------------------- |
| ItemInfoBox      | `iinf`   | **项目信息定义** | 定义项目 ID 和类型（grid/iovl）          |
| ItemLocationBox  | `iloc`   | **数据位置索引** | 记录 grid/iovl 数据在 idat/mdat 中的偏移 |
| ItemDataBox      | `idat`   | **内联数据存储** | 直接包含 grid/iovl 的二进制数据          |
| ItemReferenceBox | `iref`   | **项目间引用**   | 管理 grid/iovl 引用的子图像（**dimg**）  |

​	值得注意的是，`dimg(Drived Image Reference)`不是一种图像类型，而是**引用关系类型**，其本质是在 `ItemReferenceBox (iref)` 中定义的特殊引用类型，作用是标记衍生图像(grid/iovl)所依赖的基础图像。

* `dimg`引用示例：

```c++
MetaBox
├── iinf (项目信息)
│   ├── ID=1: Type="grid"  // 网格派生图像
│   ├── ID=11: Type="hvc1" // 基础图(左上)
│   ├── ID=12: Type="hvc1" // 基础图(中上)
│   └── ...(其他9个基础图)
└── iref (项目引用)
    └── dimg引用组:
        Source=1 → Targets=[11,12,13,21,22,23,31,32,33]
```

* `iinf`数据流示例：

```shell
meta Box
├── iinf Box
│   └── Entry #42: {item_ID=42, item_type="grid"} // 声明grid项目
├── iloc Box
│   └── Location: {item_ID=42, offset=0x120, length=24} // 数据位置
└── idat Box
    └── [0x120] 00 01 02 03... // 实际二进制数据(由parseImageGrid解析)
```

* 为什么这样设计：
  * 解耦元数据与数据：`iinf`定义项目属性，`iloc`定位数据，`idat/mdat`存储数据。
  * 高效检索：不需要解析整个数据流即可获取项目类型，可以通过`iinf`快速过滤出`grid/iovl`项目
  * 灵活存储：小数据可直接内联在`idat`，大数据可外链到`mdat`。

HEIF数据的解析流程为：先通过`iinf(ItemInfoBox)获取项目类型`，在通过`iloc(ItemLocationBox)`定位数据，最终从`idat(ItemDataBox)`或`mdat(MediaDataBox)`中加载二进制流进行解析。

#### 2.4 moov box

全称电影盒movie box，继承自ISO基础媒体文件格式（BMFF）的关键Box类型，`moov`作为元数据容器，存储全局媒体描述信息，包括图像项的元数据（如`ItemInfoBox`、`ItemLocationBox`）。解码配置参数如HVCC/AVCC/JPEG配置），时间轴信息如动画帧的时序、持续时间），轨道Track定义，主要用于扩展场景，如动态图形序列或混合媒体。在HEIF中，`moov`box为非必需，它的存在标志着文件包含**时间线媒体或动态内容** 。

**静态HEIC文件的典型结构**

```c++
┌───────┐
│ ftyp  │ → 文件类型声明（如"heic"）
├───────┤
│ meta  │ → 包含所有静态图像项元数据（ItemInfoBox/ItemLocationBox等）
├───────┤
│ mdat  │ → 存储实际编码数据（如HEVC主图、缩略图二进制）
└───────┘
```

**动态HEIF文件结构**

```c++
┌───────┐
│ ftyp  │
├───────┤
│ meta  │ → 静态图像项（如主图）
├───────┤
│ moov  │ → 视频轨道定义（动态部分）
├───────┤
│ mdat  │ → 存储静态图像和动态视频数据
└───────┘
```

**动态图像序列（Animation）**

- **场景** ：HEIF文件包含多帧图像组成的动画（如Apple Live Photo的动态部分）。
- 实现 ：
  - `moov` Box定义动画的帧率、播放顺序和时间戳。
  - 图像数据通过`meta` Box或外部引用关联到`moov`中的轨道。

**混合媒体容器**

- **场景** ：HEIF文件同时包含静态图像和视频/音频轨道。
- 示例 ：
  - 主图通过`meta` Box管理（静态HEIC图像）。
  - 附加视频通过`moov` Box的`trak`（轨道）描述。

**关键子box：**

| 子Box类型                      | 作用描述                                                     |
| ------------------------------ | ------------------------------------------------------------ |
| **`mvhd` (Movie Header Box)**  | 定义全局时间参数（如时间刻度、时长）。                       |
| **`trak` (Track Box)**         | 描述单个媒体轨道（如视频轨、音频轨），包含编解码参数、时间戳映射。 |
| **`mvex` (Movie Extends Box)** | 用于分片媒体（类似MP4的Fragmented MP4）。                    |

#### 2.5 moof box

全称Movie Fragment Box，继承自ISO基础媒体文件格式（BMFF）的关键Box类型，作用于分片存储（将媒体数据分割为多个片段（Fragment），每个`moof`对应一个分片），动态流式传输。
