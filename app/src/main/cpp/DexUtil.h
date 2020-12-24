//
// https://github.com/ylcangel/android_poke/blob/a9585fd0b9bee132a04fd4731883d2ace0e16f5c/method1/src/cpp/DexUtil.cpp
// 但安卓8.0 dex_file结构体已经发生变化。。偏移量计算不准，但是思路很棒
//
// dex_file结构体采用了:https://github.com/KB5201314/ZjDroid/blob/15b9bf4fb5a5b62c9c5dc2402f04bdaa629b22ad/app/src/main/jni/dvmnative/dexfile_art.h
// 与系统源码类似http://androidxref.com/8.0.0_r4/xref/art/runtime/dex_file.h
//
// BTW：莫不是每个安卓系统版本都需要兼容。。。
//
#ifndef __DEX_UTIL_H__
#define __DEX_UTIL_H__

#include <assert.h>

#include "Types.h"
#include "log.h"

/* DEX file magic number */
#define DEX_MAGIC       "dex\n"

/* current version, encoded in 4 bytes of ASCII */
#define DEX_MAGIC_VERS  "036\0"

/*
 * older but still-recognized version (corresponding to Android API
 * levels 13 and earlier
 */
#define DEX_MAGIC_VERS_API_13  "035\0"

/* same, but for optimized DEX header */
#define DEX_OPT_MAGIC   "dey\n"
#define DEX_OPT_MAGIC_VERS  "036\0"

#define DEX_DEP_MAGIC   "deps"

#define VALID_METHOD_ID 0xffffffff

enum { kSHA1DigestLen = 20,
    kSHA1DigestOutputLen = kSHA1DigestLen*2 +1 };

/*
 * Header added by DEX optimization pass.  Values are always written in
 * local byte and structure padding.  The first field (magic + version)
 * is guaranteed to be present and directly readable for all expected
 * compiler configurations; the rest is version-dependent.
 *
 * Try to keep this simple and fixed-size.
 */
struct DexOptHeader {
    u1  magic[8];           /* includes version number */

    u4  dexOffset;          /* file offset of DEX header */
    u4  dexLength;
    u4  depsOffset;         /* offset of optimized DEX dependency table */
    u4  depsLength;
    u4  optOffset;          /* file offset of optimized data tables */
    u4  optLength;

    u4  flags;              /* some info flags */
    u4  checksum;           /* adler32 checksum covering deps/opt */

    /* pad for 64-bit alignment if necessary */
};


/*
 * Lookup table for classes.  It provides a mapping from class name to
 * class definition.  Used by dexFindClass().
 *
 * We calculate this at DEX optimization time and embed it in the file so we
 * don't need the same hash table in every VM.  This is slightly slower than
 * a hash table with direct pointers to the items, but because it's shared
 * there's less of a penalty for using a fairly sparse table.
 */
struct DexClassLookup {
    int     size;                       // total size, including "size"
    int     numEntries;                 // size of table[]; always power of 2
    struct {
        u4      classDescriptorHash;    // class descriptor hash code
        int     classDescriptorOffset;  // in bytes, from start of DEX
        int     classDefOffset;         // in bytes, from start of DEX
    } table[1];
};

/*
 * Direct-mapped "header_item" struct.
 */
struct DexHeader {
    u1  magic[8];           /* includes version number */
    u4  checksum;           /* adler32 checksum */
    u1  signature[kSHA1DigestLen]; /* SHA-1 hash */
    u4  fileSize;           /* length of entire file */
    u4  headerSize;         /* offset to start of next section */
    u4  endianTag;
    u4  linkSize;
    u4  linkOff;
    u4  mapOff;
    u4  stringIdsSize;
    u4  stringIdsOff;
    u4  typeIdsSize;
    u4  typeIdsOff;
    u4  protoIdsSize;
    u4  protoIdsOff;
    u4  fieldIdsSize;
    u4  fieldIdsOff;
    u4  methodIdsSize;
    u4  methodIdsOff;
    u4  classDefsSize;
    u4  classDefsOff;
    u4  dataSize;
    u4  dataOff;
};

/* map item type codes */
enum {
    kDexTypeHeaderItem               = 0x0000,
    kDexTypeStringIdItem             = 0x0001,
    kDexTypeTypeIdItem               = 0x0002,
    kDexTypeProtoIdItem              = 0x0003,
    kDexTypeFieldIdItem              = 0x0004,
    kDexTypeMethodIdItem             = 0x0005,
    kDexTypeClassDefItem             = 0x0006,
    kDexTypeMapList                  = 0x1000,
    kDexTypeTypeList                 = 0x1001,
    kDexTypeAnnotationSetRefList     = 0x1002,
    kDexTypeAnnotationSetItem        = 0x1003,
    kDexTypeClassDataItem            = 0x2000,
    kDexTypeCodeItem                 = 0x2001,
    kDexTypeStringDataItem           = 0x2002,
    kDexTypeDebugInfoItem            = 0x2003,
    kDexTypeAnnotationItem           = 0x2004,
    kDexTypeEncodedArrayItem         = 0x2005,
    kDexTypeAnnotationsDirectoryItem = 0x2006,
};


/*
 * access flags and masks; the "standard" ones are all <= 0x4000
 *
 * Note: There are related declarations in vm/oo/Object.h in the ClassFlags
 * enum.
 */
enum {
    ACC_PUBLIC       = 0x00000001,       // class, field, method, ic
    ACC_PRIVATE      = 0x00000002,       // field, method, ic
    ACC_PROTECTED    = 0x00000004,       // field, method, ic
    ACC_STATIC       = 0x00000008,       // field, method, ic
    ACC_FINAL        = 0x00000010,       // class, field, method, ic
    ACC_SYNCHRONIZED = 0x00000020,       // method (only allowed on natives)
    ACC_SUPER        = 0x00000020,       // class (not used in Dalvik)
    ACC_VOLATILE     = 0x00000040,       // field
    ACC_BRIDGE       = 0x00000040,       // method (1.5)
    ACC_TRANSIENT    = 0x00000080,       // field
    ACC_VARARGS      = 0x00000080,       // method (1.5)
    ACC_NATIVE       = 0x00000100,       // method
    ACC_INTERFACE    = 0x00000200,       // class, ic
    ACC_ABSTRACT     = 0x00000400,       // class, method, ic
    ACC_STRICT       = 0x00000800,       // method
    ACC_SYNTHETIC    = 0x00001000,       // field, method, ic
    ACC_ANNOTATION   = 0x00002000,       // class, ic (1.5)
    ACC_ENUM         = 0x00004000,       // class, field, ic (1.5)
    ACC_CONSTRUCTOR  = 0x00010000,       // method (Dalvik only)
    ACC_DECLARED_SYNCHRONIZED =
    0x00020000,       // method (Dalvik only)
    ACC_CLASS_MASK =
    (ACC_PUBLIC | ACC_FINAL | ACC_INTERFACE | ACC_ABSTRACT
     | ACC_SYNTHETIC | ACC_ANNOTATION | ACC_ENUM),
    ACC_INNER_CLASS_MASK =
    (ACC_CLASS_MASK | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC),
    ACC_FIELD_MASK =
    (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL
     | ACC_VOLATILE | ACC_TRANSIENT | ACC_SYNTHETIC | ACC_ENUM),
    ACC_METHOD_MASK =
    (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL
     | ACC_SYNCHRONIZED | ACC_BRIDGE | ACC_VARARGS | ACC_NATIVE
     | ACC_ABSTRACT | ACC_STRICT | ACC_SYNTHETIC | ACC_CONSTRUCTOR
     | ACC_DECLARED_SYNCHRONIZED),
};

/*
 * Enumeration of all the primitive types.
 */
enum PrimitiveType {
    PRIM_NOT        = 0,       /* value is a reference type, not a primitive type */
    PRIM_VOID       = 1,
    PRIM_BOOLEAN    = 2,
    PRIM_BYTE       = 3,
    PRIM_SHORT      = 4,
    PRIM_CHAR       = 5,
    PRIM_INT        = 6,
    PRIM_LONG       = 7,
    PRIM_FLOAT      = 8,
    PRIM_DOUBLE     = 9,
};

/*
 * Direct-mapped "map_item".
 */
struct DexMapItem {
    u2 type;              /* type code (see kDexType* above) */
    u2 unused;
    u4 size;              /* count of items of the indicated type */
    u4 offset;            /* file offset to the start of data */
};

/*
 * Direct-mapped "map_list".
 */
struct DexMapList {
    u4  size;               /* #of entries in list */
    DexMapItem list[1];     /* entries */
};

/*
 * Direct-mapped "string_id_item".
 */
struct DexStringId {
    u4 stringDataOff;      /* file offset to string_data_item */
};

/*
 * Direct-mapped "type_id_item".
 */
struct DexTypeId {
    u4  descriptorIdx;      /* index into stringIds list for type descriptor */
};

/*
 * Direct-mapped "field_id_item".
 */
struct DexFieldId {
    u2  classIdx;           /* index into typeIds list for defining class */
    u2  typeIdx;            /* index into typeIds for field type */
    u4  nameIdx;            /* index into stringIds for field name */
};

/*
 * Direct-mapped "method_id_item".
 */
struct DexMethodId {
    u2  classIdx;           /* index into typeIds list for defining class */
    u2  protoIdx;           /* index into protoIds for method prototype */
    u4  nameIdx;            /* index into stringIds for method name */
};

/*
 * Direct-mapped "proto_id_item".
 */
struct DexProtoId {
    u4  shortyIdx;          /* index into stringIds for shorty descriptor */
    u4  returnTypeIdx;      /* index into typeIds list for return type */
    u4  parametersOff;      /* file offset to type_list for parameter types */
};

/*
 * Direct-mapped "class_def_item".
 */
struct DexClassDef {
    u4  classIdx;           /* index into typeIds for this class */
    u4  accessFlags;
    u4  superclassIdx;      /* index into typeIds for superclass */
    u4  interfacesOff;      /* file offset to DexTypeList */
    u4  sourceFileIdx;      /* index into stringIds for source file name */
    u4  annotationsOff;     /* file offset to annotations_directory_item */
    u4  classDataOff;       /* file offset to class_data_item */
    u4  staticValuesOff;    /* file offset to DexEncodedArray */
};

/*
 * Direct-mapped "code_item".
 *
 * The "catches" table is used when throwing an exception,
 * "debugInfo" is used when displaying an exception stack trace or
 * debugging. An offset of zero indicates that there are no entries.
 */
struct DexCode {
    u2  registersSize;
    u2  insSize;
    u2  outsSize;
    u2  triesSize;
    u4  debugInfoOff;       /* file offset to debug info stream */
    u4  insnsSize;          /* size of the insns array, in u2 units */
    u2  insns[1];
    /* followed by optional u2 padding */
    /* followed by try_item[triesSize] */
    /* followed by uleb128 handlersSize */
    /* followed by catch_handler_item[handlersSize] */
};

/*
 * Direct-mapped "try_item".
 */
struct DexTry {
    u4  startAddr;          /* start address, in 16-bit code units */
    u2  insnCount;          /* instruction count, in 16-bit code units */
    u2  handlerOff;         /* offset in encoded handler data to handlers */
};

/*
 * Link table.  Currently undefined.
 */
struct DexLink {
    u1  bleargh;
};

/*
 * Direct-mapped "type_item".
 */
struct DexTypeItem {
    u2  typeIdx;            /* index into typeIds */
};

/*
 * Direct-mapped "type_list".
 */
struct DexTypeList {
    u4  size;               /* #of entries in list */
    DexTypeItem list[1];    /* entries */
};

/*
 * Direct-mapped "annotations_directory_item".
 */
struct DexAnnotationsDirectoryItem {
    u4  classAnnotationsOff;  /* offset to DexAnnotationSetItem */
    u4  fieldsSize;           /* count of DexFieldAnnotationsItem */
    u4  methodsSize;          /* count of DexMethodAnnotationsItem */
    u4  parametersSize;       /* count of DexParameterAnnotationsItem */
    /* followed by DexFieldAnnotationsItem[fieldsSize] */
    /* followed by DexMethodAnnotationsItem[methodsSize] */
    /* followed by DexParameterAnnotationsItem[parametersSize] */
};

/*
 * Direct-mapped "field_annotations_item".
 */
struct DexFieldAnnotationsItem {
    u4  fieldIdx;
    u4  annotationsOff;             /* offset to DexAnnotationSetItem */
};

/*
 * Direct-mapped "method_annotations_item".
 */
struct DexMethodAnnotationsItem {
    u4  methodIdx;
    u4  annotationsOff;             /* offset to DexAnnotationSetItem */
};

/*
 * Direct-mapped "parameter_annotations_item".
 */
struct DexParameterAnnotationsItem {
    u4  methodIdx;
    u4  annotationsOff;             /* offset to DexAnotationSetRefList */
};

/*
 * Direct-mapped "annotation_set_ref_item".
 */
struct DexAnnotationSetRefItem {
    u4  annotationsOff;             /* offset to DexAnnotationSetItem */
};

/*
 * Direct-mapped "annotation_set_ref_list".
 */
struct DexAnnotationSetRefList {
    u4  size;
    DexAnnotationSetRefItem list[1];
};

/*
 * Direct-mapped "annotation_set_item".
 */
struct DexAnnotationSetItem {
    u4  size;
    u4  entries[1];                 /* offset to DexAnnotationItem */
};

/* expanded form of a class_data_item header */
struct DexClassDataHeader {
    u4 staticFieldsSize;
    u4 instanceFieldsSize;
    u4 directMethodsSize;
    u4 virtualMethodsSize;
};

/* expanded form of encoded_field */
struct DexField {
    u4 fieldIdx;    /* index to a field_id_item */
    u4 accessFlags;
};

/* expanded form of encoded_method */
struct DexMethod {
    u4 methodIdx;    /* index to a method_id_item */
    u4 accessFlags;
    u4 codeOff;      /* file offset to a code_item */
};

/* expanded form of class_data_item. Note: If a particular item is
 * absent (e.g., no static fields), then the corresponding pointer
 * is set to nullptr. */
struct DexClassData {
    DexClassDataHeader header;
    DexField*          staticFields;
    DexField*          instanceFields;
    DexMethod*         directMethods;
    DexMethod*         virtualMethods;
};

struct PaddingData {
    u4 methodIdx;
    u4 protoIdx;
    u4 codeOff;
    u4 insnsSize;
    const u1* codePtr;
};

/*
 * Structure representing a DEX file.
 *
 * Code should regard DexFile as opaque, using the API calls provided here
 * to access specific structures.
 */
struct DexFile {
    /* directly-mapped "opt" header */
    const DexOptHeader* pOptHeader;

    /* pointers to directly-mapped structs and arrays in base DEX */
    const DexHeader*    pHeader;
    const DexStringId*  pStringIds;
    const DexTypeId*    pTypeIds;
    const DexFieldId*   pFieldIds;
    const DexMethodId*  pMethodIds;
    const DexProtoId*   pProtoIds;
    const DexClassDef*  pClassDefs;
    const DexLink*      pLinkData;

    /*
     * These are mapped out of the "auxillary" section, and may not be
     * included in the file.
     */
    const DexClassLookup* pClassLookup;
    const void*         pRegisterMapPool;       // RegisterMapClassPool

    /* points to start of DEX file data */
    const u1*           baseAddr;

    /* track memory overhead for auxillary structures */
    int                 overhead;

    /* additional app-specific data structures associated with the DEX */
    //void*               auxData;
};

/*
 * Map constant pool indices from one form to another.  Some or all of these
 * may be nullptr.
 *
 * The map values are 16-bit unsigned values.  If the values we map to
 * require a larger range, we omit the mapping for that category (which
 * requires that the lookup code recognize that the data will not be
 * there for all DEX files in all categories.)
 *
 *
 * 2.2.3 and less
 */
typedef struct DexIndexMap {
    const u2* classMap;         /* map, either expanding or reducing */
    u4  classFullCount;         /* same as typeIdsSize */
    u4  classReducedCount;      /* post-reduction count */
    const u2* methodMap;
    u4  methodFullCount;
    u4  methodReducedCount;
    const u2* fieldMap;
    u4  fieldFullCount;
    u4  fieldReducedCount;
    const u2* stringMap;
    u4  stringFullCount;
    u4  stringReducedCount;
} DexIndexMap;

/*
 * Structure representing a DEX file.
 *
 * 2.2.3 and less
 */
struct DexFile2X {
    /* directly-mapped "opt" header */
    const DexOptHeader* pOptHeader;

    /* pointers to directly-mapped structs and arrays in base DEX */
    const DexHeader*    pHeader;
    const DexStringId*  pStringIds;
    const DexTypeId*    pTypeIds;
    const DexFieldId*   pFieldIds;
    const DexMethodId*  pMethodIds;
    const DexProtoId*   pProtoIds;
    const DexClassDef*  pClassDefs;
    const DexLink*      pLinkData;

    /*
     * These are mapped out of the "auxillary" section, and may not be
     * included in the file.
     */
    const DexClassLookup* pClassLookup;
    DexIndexMap         indexMap;
    const void*         pRegisterMapPool;       // RegisterMapClassPool

    /* points to start of DEX file data */
    const u1*           baseAddr;

    /* track memory overhead for auxillary structures */
    int                 overhead;

    /* additional app-specific data structures associated with the DEX */
    //void*               auxData;
};

class UpdateClassDataFilter {

public:
    virtual ~UpdateClassDataFilter(){}
    virtual bool onMethod(u4 classId, u4 methodId, u4 methodIndex, DexMethod& dexMethod) = 0;
};

class DexUtil {
public:
    DexUtil(const u1* addr);
    ~DexUtil();

public:
    static bool isDex(const u1* addr);
    static bool isOptDex(const u1* addr);
    static u4 getNativeCountInDexClassData(DexClassData* classDataItem);

public:

/*
* Get the type descriptor character associated with a given primitive
* type. This returns '\0' if the type is invalid.
*/
    char dexGetPrimitiveTypeDescriptorChar(PrimitiveType type);

/*
 * Get the type descriptor string associated with a given primitive
 * type.
 */
    const char* dexGetPrimitiveTypeDescriptor(PrimitiveType type);

/*
 * Get the boxed type descriptor string associated with a given
 * primitive type. This returns nullptr for an invalid type, including
 * particularly for type "void". In the latter case, even though there
 * is a class Void, there's no such thing as a boxed instance of it.
 */
    const char* dexGetBoxedTypeDescriptor(PrimitiveType type);

/*
 * Get the primitive type constant from the given descriptor character.
 * This returns PRIM_NOT (note: this is a 0) if the character is invalid
 * as a primitive type descriptor.
 */
    PrimitiveType dexGetPrimitiveTypeFromDescriptorChar(char descriptorChar);

public:
    const u1* base() {
        return mDexFile->baseAddr;
    }

    u4 fileSize() {
        return mDexFile->pHeader->fileSize;
    }

    u4 getTypeIdsSize() {
        return mDexFile->pHeader->typeIdsSize;
    }

    const DexHeader* header() {
        return mDexFile->pHeader;
    }

    u4 classCount() {
        return mDexFile->pHeader->classDefsSize;
    }

    u4 methodCount() {
        return mDexFile->pHeader->methodIdsSize;
    }

    u4 checksum() {
        return mDexFile->pHeader->checksum;
    }

    u4 classDescriptorHash(const char* str);


    bool hasNative();
    int findClassIndex(const char* name);
    u4 getMethodCount(u4 classIdx);
    const char* getMethodName(u4 classIdx, u4 methodIndex);

    void calcMethodHash(u4 methodIdx, char* hashStr);

    DexClassLookup* dexCreateClassLookup();
    void classLookupAdd(DexClassLookup* pLookup, int stringOff, int classDefOff, int* pNumProbes);

    DexClassData* dexReadAndVerifyClassData(const u1** pData, const u1* pLimit);

    void dexFileSetupBasicPointers(DexFile* pDexFile, const u1* data);


    const DexCode* dexGetCode( const DexMethod* pDexMethod) {
        if (pDexMethod->codeOff == 0)
            return nullptr;
        return (const DexCode*) (mDexFile->baseAddr + pDexMethod->codeOff);
    }

    const u4 getDexCodeSize(const DexCode* dexCode) {
        return sizeof(DexCode) + dexCode->insnsSize;
    }

    const DexClassDef* dexGetClassDef(u4 idx) {
        assert(idx < classCount());
        return &mDexFile->pClassDefs[idx];
    }

    const DexTypeId* dexGetTypeId(u4 idx) {
        assert(idx < mDexFile->pHeader->typeIdsSize);
        return &mDexFile->pTypeIds[idx];
    }

    const char* dexStringByTypeIdx(u4 idx) {
        const DexTypeId* typeId = dexGetTypeId(idx);
        return dexStringById(typeId->descriptorIdx);
    }

    const char* dexGetStringData(const DexStringId* pStringId) {
        const u1* ptr = mDexFile->baseAddr + pStringId->stringDataOff;

        // Skip the uleb128 length.
        while (*(ptr++) > 0x7f) /* empty */ ;

        return (const char*) ptr;
    }

    const DexStringId* dexGetStringId(u4 idx) {
        assert(idx < mDexFile->pHeader->stringIdsSize);
        const DexStringId* pStringIds = (const DexStringId*)(mDexFile->pStringIds);
        return &pStringIds[idx];
    }

    const DexStringId* getDexStringIdByTypeIdx(u4 idx) {
        const DexTypeId* typeId = dexGetTypeId(idx);
        return dexGetStringId(typeId->descriptorIdx);
    }

    const char* dexStringById(u4 idx) {
        const DexStringId* pStringId = dexGetStringId(idx);
        return dexGetStringData(pStringId);
    }

    const DexTypeItem* dexGetTypeItem(const DexTypeList* pList, u4 idx) {
        assert(idx < pList->size);
        return &pList->list[idx];
    }

    const DexMethodId* dexGetMethodId(u4 idx) {
        assert(idx < mDexFile->pHeader->methodIdsSize);
        const DexMethodId* pMethodIds = (const DexMethodId*) (mDexFile->pMethodIds);
        return &pMethodIds[idx];
    }

    const DexProtoId* dexGetProtoId(u4 idx) {
        const DexProtoId* pProtoIds = (const DexProtoId*) (mDexFile->pProtoIds);
        return &pProtoIds[idx];
    }

    const DexTypeList* dexGetProtoParameters(const DexProtoId* pProtoId) {
        if (pProtoId->parametersOff == 0) {
            return nullptr;
        }
        return (const DexTypeList*)(mDexFile->baseAddr + pProtoId->parametersOff);
    }

    const u1* dexGetClassData(const DexClassDef& classDef) const {
        if (classDef.classDataOff == 0) {
            return nullptr;
        } else {
            return mDexFile->baseAddr + classDef.classDataOff;
        }
    }

    const DexFile* getDexFile() {
        return mDexFile;
    }

private:
    DexFile* mDexFile;
};

#endif // __DEX_UTIL_H__
