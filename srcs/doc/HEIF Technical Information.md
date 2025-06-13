## HEIF Technical Information

> reference: https://nokiatech.github.io/heif/technical.html

### HEIF的基本设计原则：

* 基于ISOBMFF（ISO Base Media File Format）标准，对于静态图像，以 items 的形式存储，每个image items独立编码，不对外部依赖；对于图像序列，以 tracks的形式存储，当 image之间存在依赖时，可以用 image sequence track的形式存储，而不是image items。

* HEIF格式的文件有一系列 boxes 文件结构组成，其中boxes 包含4字节的box类型编码、box的大小（以字节为单位）以及box的载荷，同时box本身可以被嵌套。ISOBMFF和HEIF约束了box的顺序和结构。

* HEIF中使用FileTypeBox来说明当前Box的内容以及编码方式，FileTypeBox位于文件开头，分为不同的brands，每个brands通过唯一的四字符代码识别。

  | Brand | Coding format                                     | image or sequence | File extension |
  | ----- | ------------------------------------------------- | ----------------- | -------------- |
  | mif1  | any                                               | image             | .heif          |
  | msf1  | any                                               | sequence          | .heif          |
  | heic  | HEVC (Main or Main Still Picture profile)         | image             | .heic          |
  | heix  | HEVC (Main 10 or format range extensions profile) | image             | .heic          |
  | hevc  | HEVC (Main or Main Still Picture profile)         | sequence          | .heic          |
  | hevx  | HEVC (Main 10 or format range extensions profile) | sequence          | .heic          |

  ​	HEVC (Main or Main Still Picture配置)和HEVC (Main 10 or format range extensions配置)和区别主要在色彩精度和功能范围上，Main or Main Still Picture采用8位色深，主要用在普用照片/视频，SDR标准动态范围，兼容性好，手机/电脑/电视都能播。Main 10 or format range extensions采用10位色深，主要用在HDR高动态范围内容/专业影像，支持更广的色彩空间（eg: Rec.2020），需要支持HDR的屏幕才能展现。

* HEIF文件中可以包含多张图像，功能其功能的不同可以做一些分类，值得注意的是，一张图像可能有多个功能。

  | 功能                                 | 描述                                                         |
  | ------------------------------------ | ------------------------------------------------------------ |
  | 封面图 cover image                   | 文件外包装展示图，快速加载，常与缩略图共用                   |
  | 缩略图 thumbnail image               | 快速浏览索引，相册网络视图中的小图，极低分辨率               |
  | 辅助图 auxiliary image               | 增强主图功能，如深度图、透明度通道                           |
  | 主图 master image                    | 原始高质量版本，如单反相机的RAW文件，最高分辨率/无压缩       |
  | 隐藏图 hidden image                  | 非可视化数据存储，入照片水印、版权元数据等                   |
  | 预生成编码图 pre-derived coded image | 提前准备常用尺寸，如微信预览图（1080x720）、网页展示图（800x600） |
  | 编码图 coded image                   | 实际存储的图像数据，如HEVC压缩后的照片主体                   |
  | 衍生图像 derived image               | 实时生成的应用图像，如美艳滤镜、智能放大后的4K图             |

* 图像属性

  ​	HEIF文件的图像属性保存在 ItemPropertyContainerBox，其分为两类：descriptive和transformative，descriptive属性不会应用到图像的修改上，而transformative则会，除了通过descriptive image properties，HEIF也可以通过metadata items来表征image items，metadata items的格式有：Exif、XMP、MPEG-7 metadata。

  | Name                                           | Type           | Description                                                  |
  | ---------------------------------------------- | -------------- | ------------------------------------------------------------ |
  | Decoder configuration and initialization       | Descriptive    | 初始化解码器所需的信息。该信息的结构通常在相关的图像编码格式规范中定义。 |
  | Image spatial Extents (‘ispe’)                 | Descriptive    | 表示关联图像项的宽度和高度                                   |
  | Pixel Aspect Ratio (‘pasp’)                    | Descriptive    | 具有与 ISO/IEC 14496-12 中定义的 PixelAspectRatioBox 相同的语法。 |
  | Color Information (‘colr’)                     | Descriptive    | 与 ISO/IEC 14496-12 中定义的 ColourInformationBox 具有相同的语法。 |
  | Pixel Information (‘pixi’)                     | Descriptive    | 表示关联图像项的重建图像中颜色分量的数量和位深度。           |
  | Relative Location (‘rloc’)                     | Descriptive    | 表示重建图像项相对于关联图像项的水平和垂直偏移。             |
  | Image Properties for Auxiliary Images (‘auxC’) | Descriptive    | 辅助图像必须与 AuxiliaryTypeProperty 相关联。                |
  | Content light level (‘clli’)                   | Descriptive    | 与 ISO/IEC 14496-12 中定义的 ContentLightLevelBox 具有相同的语法。 |
  | Mastering display colour volume (‘mdcv’)       | Descriptive    | 与 ISO/IEC 14496-12 中定义的 MasteringDisplayColourVolumeBox 具有相同的语法。 |
  | Content colour volume (‘cclv’)                 | Descriptive    | 与 ISO/IEC 14496-12 中定义的 ContentColourVolumeBox 具有相同的语法。 |
  | Required reference types (‘rrtp’)              | Descriptive    | Lists the item reference types that a reader shall process to display the associated image item. |
  | Creation time information (‘crtt’)             | Descriptive    | Creation time of the associated item or entity group.        |
  | Modification time information (‘mdft‘)         | Descriptive    | Modification time of the associated item or entity group.    |
  | User description (‘udes‘)                      | Descriptive    | 用户定义的名称、描述和标签。                                 |
  | Accessibility text (‘altt‘)                    | Descriptive    | Alternate text for an image, in case the image cannot be displayed. |
  | Auto exposure information (‘aebr‘)             | Descriptive    | 在包围曝光照片组中，关于各张照片曝光差异的信息               |
  | White balance information (‘wbbr‘)             | Descriptive    | 在包围曝光照片组中，关于白平衡补偿信息                       |
  | Focus information (‘fobr‘)                     | Descriptive    | 在包围曝光照片组中，焦点变化信息                             |
  | Flash exposure information (‘afbr‘)            | Descriptive    | 在包围曝光照片组中，闪光灯曝光变化信息                       |
  | Depth of field information (‘dobr‘)            | Descriptive    | 在包围曝光照片组中，景深变化信息                             |
  | Panorama information (‘pano‘)                  | Descriptive    | Characteristics about the associated panorama entity group.  |
  | Image Scaling (‘iscl’)                         | Transformative | 将输入图像的大小调整为目标宽度和高度。                       |
  | Image Rotation (‘irot’)                        | Transformative | 旋转 90 度、180 度或 270 度。                                |
  | Clean Aperture (‘clap’)                        | Transformative | 根据给定的裁剪矩形进行裁剪。                                 |

* Derived Images

  衍生图像可以实现非破坏性的图像编辑功能，HEIF指定了用于将衍生图像保存为items的通用结构，也定义了一些特定类型的衍生图像。衍生图像本身也可以有descriptive和transformative属性，type item: 'dimg'指明了衍生图像的类型。

  | Name                             | Description                                                  |
  | -------------------------------- | ------------------------------------------------------------ |
  | Identity transformation (‘iden’) | 通过相应的变换属性进行裁剪和/或旋转 90 度、180 度或 270 度。 |
  | Image Overlay (‘iovl’)           | 将任意数量的输入图像按指定的顺序和位置叠加到输出图像的画布上。 |
  | Image Grid (‘grid’)              | 重建具有相同宽度和高度的输入图像网格。                       |

* Predictively Coded Images

  如果一个图像块解码时需要依赖其他图像块，就用'pred'标记，预测编码图像项的解码依赖关系和顺序由 item references of type 'pred'来指定。

* Image Metadata

  HEIF文件允许存储与图像和图像序列相关的metadata，此类数据可以是与完整性检查、EXIF 或 XMP 数据或 MPEG-7 相关元数据相关的信息。对于image items，元数据metadata被以metadata item的形式存储并通过**'cdsc'引用**指向其描述的image item，对于image sequences，时间相关的元数据（如每帧的陀螺仪数据）会存储在独立的 timed metadata track并通过**'cdsc'引用**指向关联的image sequence track。

* Indicating Alternative Representations

​		指示替代表示，是一种为同一图像提供多种不同版本或变体的技术机制。HEIF文件可以包含多种image items来表示相同的图像内容。媒体播放器可以选择同一图像内容的替代表示之一进行显示。一种称为**entity grouping**（定义在GroupsListBox）的机制用于指示替代组。

* Image sequences

  image sequence tracks的作用

  | 作用                                | 描述                                                         |
  | ----------------------------------- | ------------------------------------------------------------ |
  | 缩略图序列 thumbnail image sequence | 主图像序列的较小分辨率表示                                   |
  | 辅助图序列 auxiliary image sequence | 与主图像序列互补的图像序列。                                 |
  | 主图序列 master image sequence      | 除缩略图序列或辅助图像序列之外的图像序列。通常包含可全分辨率显示的影像。 |

  HEIF 支持使用帧间预测来紧凑存储图像序列，此外image sequence的use cases可能要求更快地访问单个图像并能够编辑单个图像而不影响任何其他图像，因此HEIF包含以下2个特性：

  * CodingConstraintsBox ，
  * DirectReferenceSamplesList ，

* 文件播放器处理image items

  略

* HEIF文件的播放选项

  | Feature                      | First appeared in | Description                                                  |
  | ---------------------------- | ----------------- | ------------------------------------------------------------ |
  | non-displayable sample       | ISO/IEC 14496-15  | 永远不会显示，但可以作为预测轨道中其他图像的参考。           |
  | timed vs. non-timed playback | HEIF              | 在定时播放中，图像序列以视频形式播放，而在非定时播放中，曲目的样本以其他方式显示，例如图库 |
  | edit list                    | ISOBMFF           | 图像序列轨道按播放顺序排列的范围列表。允许修改播放顺序和样本速度。 |
  | looping                      | HEIF              | HEIF 允许指示编辑列表重复，例如用于循环动画。重复可以指示为持续一定时间或无限次 |
  | cropping and rotation        | ISOBMFF           | 可以指定矩形裁剪和90度、180度、270度旋转。                   |

* 将HEVC编码图像存储在HEIF文件中