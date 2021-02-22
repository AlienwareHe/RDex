# RDex
Xposed脱壳机，目前支持二代填充式类抽取加固

# Feature
* 支持安卓8.0-10.0，根据mcookie dump对应的dex_file
* 安卓7.1及之前与Fdex2原理一致


因为安卓7.1之后没有了Dex#getBytes方法，所以基于该方法的脱壳插件例如FDex2等都无法运行。因此思考如何兼容安卓8.0+

ClassLoader加载源码过程：https://www.infoq.cn/article/rczbnbwtff_qyjrqieyl

# 兼容思路
目前不修改ROM的情况下的脱壳机主要是两种流派，一种通过Hook得到DexFile，例如FART等，一种通过Classloader和Dex#getBytes方法得到Dex数据，例如FDex。

这里延续FDex的思路，期望在安卓8、9、10上继续完成脱壳，因此在通过遍历classloader拿到对应的dexElement之后，开始想如何弥补Dex#getBytes方法不存在的空缺。

熟悉脱壳的人应该都知道，DexElement中有一个字段dexFile，其中包含了dex的唯一id即mCookie，该cookie在native中就代表了dexFile，可以通过mCookie获取到对应的DexFile，源码可见：
https://github.com/anestisb/vdexExtractor#compact-dex-converter

而且这个方法很简单，因此我们便可以通过自己实现该方法来获取对应的DexFile，然后根据DexFile中的偏移dump出对应的Dex。

# 实现思路
寒冰大佬在找看雪的ART脱壳Hook点提到过：

Android APP脱壳的本质就是对内存中处于解密状态的dex的dump。首先要区分这里的脱壳和修复的区别。这里的脱壳指的是对加固apk中保护的dex的整体的dump，不管是函数抽取、dex2c还是vmp壳，首要做的就是对整体dex的dump，然后再对脱壳下来的dex进行修复。要达到对apk的脱壳，最为关键的就是准确定位内存中解密后的dex文件的起始地址和大小。那么这里要达成对apk的成功脱壳，就有两个最为关键的要素：

1、内存中dex的起始地址和大小，只有拿到这两个要素，才能够成功dump下内存中的dex

2、脱壳时机，只有正确的脱壳时机，才能够dump下明文状态的dex。否则，时机不对，及时是正确的起始地址和大小，dump下来的也可能只是密文。

所以脱壳机核心是时机和拿到Dex文件。

在8.0之前Java层有Dex#getBytes方法可以拿到Dex文件，但8.0之后便无了，

但是分析ART中ClassLoader加载一个Class文件的源码，可以得知Class类中有一个mCookie字段用于表示Dex。

因此如果能够在Java层或者Native根据mCookie找到对应的DexFile那么脱壳的方法也就找到了，分析mCookie生成的源码：

http://aospxref.com/android-8.1.0_r81/xref/art/runtime/native/dalvik_system_DexFile.cc#ConvertDexFilesToJavaArray

可以发现在8.1中，mCookie是一个jlongArray数组，而且art还提供了ConvertJavaArrayToDexFiles方法用来将mCookie还原为DexFile数组，而且逻辑很简单，就是遍历然后强转，因此我们可以自己实现该方法（可见代码native-lib.cpp#toDexFiles_8_0），用来将mCookie转化为DexFile，然后再根据_begin和_size的偏移量获取dexFile的起始位置和大小，便可完成dex dump。


# 安卓9/10问题
## TODO 禁止dex优化

在安卓9中为了减少内存的使用量，引入了[cdex](http://aospxref.com/android-9.0.0_r61/xref/art/libdexfile/dex/compact_dex_file.h)，magic为cdex001，这导致我们dump出来的dex不是一个正常的Dex结构，无法被jadx等反编译。

解决该问题个人有两个思路，一个是将其转化为dex，例如https://github.com/anestisb/vdexExtractor#compact-dex-converter所做，该工具是一个命令行工具，对于咱们APP脱壳来说可以考虑http://aospxref.com/android-9.0.0_r61/xref/art/openjdkjvmti/fixed_up_dex_file.cc，直接在ART中转化会更加方便，但是水平有限，未能看破实现。

因此换了第二个思路，曲线救国，既然无法转化，那么是否可以禁止生成这种cdex格式文件，众所周知，art的出现对dexopt的影响是引入了AOT，dex2oat就是一个专门用于优化dex执行速度的程序，在各个版本中dex2oat的执行时机和结果都有所区别，cdex也是dex2oat的结果产物，因此我们可以考虑在代码中禁止dex2oat，即hook execve在执行dex2oat时直接返回，但是对于进程内hook的脱壳机来说该方法只能实现app运行中的dex2oat的禁止，无法实现全局。

但好在似乎头几次使用APP时并不会有dex优化生成cdex的情况，因此脱壳时可尽量保证APP第一次安装运行。

![image](http://oss.alienhe.cn/20210112234653.png)