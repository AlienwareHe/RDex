//
// Created by alienhe on 2020/12/24.
//
#include <jni.h>
#include <memory>
#include <string>
#include <stdint.h>
#include <iosfwd>
#include <limits>
#include <utility>
#include <inttypes.h>

#ifndef RDEX_DEXFILE_ART_10_H
#define RDEX_DEXFILE_ART_10_H


namespace art {

    class OatDexFile;

    class DexFileContainer {
    public:
        DexFileContainer() { }
        virtual ~DexFileContainer() { }
        virtual int GetPermissions() = 0;
        virtual bool IsReadOnly() = 0;
        virtual bool EnableWrite() = 0;
        virtual bool DisableWrite() = 0;

    private:
        DISALLOW_COPY_AND_ASSIGN(DexFileContainer);
    };

    namespace hiddenapi {
// List of domains supported by the hidden API access checks. Domain with a lower
// ordinal is considered more "trusted", i.e. always allowed to access members of
// domains with a greater ordinal. Access checks are performed when code tries to
// access a method/field from a more trusted domain than itself.
        enum class Domain : char {
            kCorePlatform = 0,
            kPlatform,
            kApplication,
        };

        inline bool IsDomainMoreTrustedThan(Domain domainA, Domain domainB) {
            return static_cast<char>(domainA) <= static_cast<char>(domainB);
        }
    }  // namespace hiddenapi

    namespace dex {

        constexpr uint32_t kDexNoIndex = 0xFFFFFFFF;

        template<typename T>
        class DexIndex {
        public:
            T index_;

            constexpr DexIndex() : index_(std::numeric_limits<decltype(index_)>::max()) {}

            explicit constexpr DexIndex(T idx) : index_(idx) {}

            bool IsValid() const {
                return index_ != std::numeric_limits<decltype(index_)>::max();
            }

            static constexpr DexIndex Invalid() {
                return DexIndex(std::numeric_limits<decltype(index_)>::max());
            }

            bool operator==(const DexIndex &other) const {
                return index_ == other.index_;
            }

            bool operator!=(const DexIndex &other) const {
                return index_ != other.index_;
            }

            bool operator<(const DexIndex &other) const {
                return index_ < other.index_;
            }

            bool operator<=(const DexIndex &other) const {
                return index_ <= other.index_;
            }

            bool operator>(const DexIndex &other) const {
                return index_ > other.index_;
            }

            bool operator>=(const DexIndex &other) const {
                return index_ >= other.index_;
            }
        };

        class ProtoIndex : public DexIndex<uint16_t> {
        public:
            ProtoIndex() {}

            explicit constexpr ProtoIndex(uint16_t index) : DexIndex<decltype(index_)>(index) {}

            static constexpr ProtoIndex Invalid() {
                return ProtoIndex(std::numeric_limits<decltype(index_)>::max());
            }
        };

        std::ostream &operator<<(std::ostream &os, const ProtoIndex &index);

        class StringIndex : public DexIndex<uint32_t> {
        public:
            StringIndex() {}

            explicit constexpr StringIndex(uint32_t index) : DexIndex<decltype(index_)>(index) {}

            static constexpr StringIndex Invalid() {
                return StringIndex(std::numeric_limits<decltype(index_)>::max());
            }
        };

        std::ostream &operator<<(std::ostream &os, const StringIndex &index);

        class TypeIndex : public DexIndex<uint16_t> {
        public:
            TypeIndex() {}

            explicit constexpr TypeIndex(uint16_t index) : DexIndex<uint16_t>(index) {}

            static constexpr TypeIndex Invalid() {
                return TypeIndex(std::numeric_limits<decltype(index_)>::max());
            }
        };

        std::ostream &operator<<(std::ostream &os, const TypeIndex &index);

    }  // namespace dex


    class DexWriter;


    namespace dex {

        struct MapItem {
            uint16_t type_;
            uint16_t unused_;
            uint32_t size_;
            uint32_t offset_;
        };

        struct MapList {
            uint32_t size_;
            MapItem list_[1];

            size_t Size() const { return sizeof(uint32_t) + (size_ * sizeof(MapItem)); }

        private:
        };

// Raw string_id_item.
        struct StringId {
            uint32_t string_data_off_;  // offset in bytes from the base address

        private:
        };

// Raw type_id_item.
        struct TypeId {
            dex::StringIndex descriptor_idx_;  // index into string_ids

        private:
        };

// Raw field_id_item.
        struct FieldId {
            dex::TypeIndex class_idx_;   // index into type_ids_ array for defining class
            dex::TypeIndex type_idx_;    // index into type_ids_ array for field type
            dex::StringIndex name_idx_;  // index into string_ids_ array for field name

        private:
        };

// Raw proto_id_item.
        struct ProtoId {
            dex::StringIndex shorty_idx_;     // index into string_ids array for shorty descriptor
            dex::TypeIndex return_type_idx_;  // index into type_ids array for return type
            uint16_t pad_;                    // padding = 0
            uint32_t parameters_off_;         // file offset to type_list for parameter types

        private:
        };

// Raw method_id_item.
        struct MethodId {
            dex::TypeIndex class_idx_;   // index into type_ids_ array for defining class
            dex::ProtoIndex proto_idx_;  // index into proto_ids_ array for method prototype
            dex::StringIndex name_idx_;  // index into string_ids_ array for method name

        private:
        };

// Base code_item, compact dex and standard dex have different code item layouts.
        struct CodeItem {
        protected:
            CodeItem() = default;

        private:
        };

// Raw class_def_item.
        struct ClassDef {
            dex::TypeIndex class_idx_;  // index into type_ids_ array for this class
            uint16_t pad1_;  // padding = 0
            uint32_t access_flags_;
            dex::TypeIndex superclass_idx_;  // index into type_ids_ array for superclass
            uint16_t pad2_;  // padding = 0
            uint32_t interfaces_off_;  // file offset to TypeList
            dex::StringIndex source_file_idx_;  // index into string_ids_ for source file name
            uint32_t annotations_off_;  // file offset to annotations_directory_item
            uint32_t class_data_off_;  // file offset to class_data_item
            uint32_t static_values_off_;  // file offset to EncodedArray

        private:
        };

// Raw type_item.
        struct TypeItem {
            dex::TypeIndex type_idx_;  // index into type_ids section

        private:
        };

// Raw type_list.
        class TypeList {
        public:
            uint32_t Size() const {
                return size_;
            }

            const TypeItem &GetTypeItem(uint32_t idx) const {
                return this->list_[idx];
            }

            // Size in bytes of the part of the list that is common.
            static constexpr size_t GetHeaderSize() {
                return 4U;
            }

            // Size in bytes of the whole type list including all the stored elements.
            static constexpr size_t GetListSize(size_t count) {
                return GetHeaderSize() + sizeof(TypeItem) * count;
            }

        private:
            uint32_t size_;  // size of the list, in entries
            TypeItem list_[1];  // elements of the list
        };

// raw method_handle_item
        struct MethodHandleItem {
            uint16_t method_handle_type_;
            uint16_t reserved1_;            // Reserved for future use.
            uint16_t field_or_method_idx_;  // Field index for accessors, method index otherwise.
            uint16_t reserved2_;            // Reserved for future use.
        private:
        };

// raw call_site_id_item
        struct CallSiteIdItem {
            uint32_t data_off_;  // Offset into data section pointing to encoded array items.
        private:
        };

// Raw try_item.
        struct TryItem {
            static constexpr size_t kAlignment = sizeof(uint32_t);

            uint32_t start_addr_;
            uint16_t insn_count_;
            uint16_t handler_off_;

        private:
            TryItem() = default;

            friend class ::art::DexWriter;
        };

        struct AnnotationsDirectoryItem {
            uint32_t class_annotations_off_;
            uint32_t fields_size_;
            uint32_t methods_size_;
            uint32_t parameters_size_;

        private:
        };

        struct FieldAnnotationsItem {
            uint32_t field_idx_;
            uint32_t annotations_off_;

        private:
        };

        struct MethodAnnotationsItem {
            uint32_t method_idx_;
            uint32_t annotations_off_;

        private:
        };

        struct ParameterAnnotationsItem {
            uint32_t method_idx_;
            uint32_t annotations_off_;

        private:
        };

        struct AnnotationSetRefItem {
            uint32_t annotations_off_;

        private:
        };

        struct AnnotationSetRefList {
            uint32_t size_;
            AnnotationSetRefItem list_[1];

        private:
        };

        struct AnnotationSetItem {
            uint32_t size_;
            uint32_t entries_[1];

        private:
        };

        struct AnnotationItem {
            uint8_t visibility_;
            uint8_t annotation_[1];

        private:
        };

        struct HiddenapiClassData {
            uint32_t size_;             // total size of the item
            uint32_t flags_offset_[1];  // array of offsets from the beginning of this item,

        private:
        };

    }  // namespace dex
}// namespace art

namespace std {

    template<>
    struct hash<art::dex::ProtoIndex> {
        size_t operator()(const art::dex::ProtoIndex &index) const {
            return hash<decltype(index.index_)>()(index.index_);
        }
    };

    template<>
    struct hash<art::dex::StringIndex> {
        size_t operator()(const art::dex::StringIndex &index) const {
            return hash<decltype(index.index_)>()(index.index_);
        }
    };

    template<>
    struct hash<art::dex::TypeIndex> {
        size_t operator()(const art::dex::TypeIndex &index) const {
            return hash<decltype(index.index_)>()(index.index_);
        }
    };

}  // namespace std


// http://androidos.net.cn/android/10.0.0_r6/xref/art/libdexfile/dex/dex_file.h
class DexFile_10 {
public:
    // Number of bytes in the dex file magic.
    static constexpr size_t kDexMagicSize = 4;
    static constexpr size_t kDexVersionLen = 4;

    // First Dex format version enforcing class definition ordering rules.
    static const uint32_t kClassDefinitionOrderEnforcedVersion = 37;

    static constexpr size_t kSha1DigestSize = 20;
    static constexpr uint32_t kDexEndianConstant = 0x12345678;

    // The value of an invalid index.
    static const uint16_t kDexNoIndex16 = 0xFFFF;
    static const uint32_t kDexNoIndex32 = 0xFFFFFFFF;

    // Raw header_item.
    struct Header {
        uint8_t magic_[8] = {};
        uint32_t checksum_ = 0;  // See also location_checksum_
        uint8_t signature_[kSha1DigestSize] = {};
        uint32_t file_size_ = 0;  // size of entire file
        uint32_t header_size_ = 0;  // offset to start of next section
        uint32_t endian_tag_ = 0;
        uint32_t link_size_ = 0;  // unused
        uint32_t link_off_ = 0;  // unused
        uint32_t map_off_ = 0;  // map list offset from data_off_
        uint32_t string_ids_size_ = 0;  // number of StringIds
        uint32_t string_ids_off_ = 0;  // file offset of StringIds array
        uint32_t type_ids_size_ = 0;  // number of TypeIds, we don't support more than 65535
        uint32_t type_ids_off_ = 0;  // file offset of TypeIds array
        uint32_t proto_ids_size_ = 0;  // number of ProtoIds, we don't support more than 65535
        uint32_t proto_ids_off_ = 0;  // file offset of ProtoIds array
        uint32_t field_ids_size_ = 0;  // number of FieldIds
        uint32_t field_ids_off_ = 0;  // file offset of FieldIds array
        uint32_t method_ids_size_ = 0;  // number of MethodIds
        uint32_t method_ids_off_ = 0;  // file offset of MethodIds array
        uint32_t class_defs_size_ = 0;  // number of ClassDefs
        uint32_t class_defs_off_ = 0;  // file offset of ClassDef array
        uint32_t data_size_ = 0;  // size of data section
        uint32_t data_off_ = 0;  // file offset of data section

        // Decode the dex magic version
        uint32_t GetVersion() const;
    };

    // Map item type codes.
    enum MapItemType {  // private
        kDexTypeHeaderItem = 0x0000,
        kDexTypeStringIdItem = 0x0001,
        kDexTypeTypeIdItem = 0x0002,
        kDexTypeProtoIdItem = 0x0003,
        kDexTypeFieldIdItem = 0x0004,
        kDexTypeMethodIdItem = 0x0005,
        kDexTypeClassDefItem = 0x0006,
        kDexTypeCallSiteIdItem = 0x0007,
        kDexTypeMethodHandleItem = 0x0008,
        kDexTypeMapList = 0x1000,
        kDexTypeTypeList = 0x1001,
        kDexTypeAnnotationSetRefList = 0x1002,
        kDexTypeAnnotationSetItem = 0x1003,
        kDexTypeClassDataItem = 0x2000,
        kDexTypeCodeItem = 0x2001,
        kDexTypeStringDataItem = 0x2002,
        kDexTypeDebugInfoItem = 0x2003,
        kDexTypeAnnotationItem = 0x2004,
        kDexTypeEncodedArrayItem = 0x2005,
        kDexTypeAnnotationsDirectoryItem = 0x2006,
        kDexTypeHiddenapiClassData = 0xF000,
    };

    // MethodHandle Types
    enum class MethodHandleType {  // private
        kStaticPut = 0x0000,  // a setter for a given static field.
        kStaticGet = 0x0001,  // a getter for a given static field.
        kInstancePut = 0x0002,  // a setter for a given instance field.
        kInstanceGet = 0x0003,  // a getter for a given instance field.
        kInvokeStatic = 0x0004,  // an invoker for a given static method.
        kInvokeInstance = 0x0005,  // invoke_instance : an invoker for a given instance method. This
        // can be any non-static method on any class (or interface) except
        // for “<init>”.
        kInvokeConstructor = 0x0006,  // an invoker for a given constructor.
        kInvokeDirect = 0x0007,  // an invoker for a direct (special) method.
        kInvokeInterface = 0x0008,  // an invoker for an interface method.
        kLast = kInvokeInterface
    };

    // Annotation constants.
    enum {
        kDexVisibilityBuild = 0x00,     /* annotation visibility */
        kDexVisibilityRuntime = 0x01,
        kDexVisibilitySystem = 0x02,

        kDexAnnotationByte = 0x00,
        kDexAnnotationShort = 0x02,
        kDexAnnotationChar = 0x03,
        kDexAnnotationInt = 0x04,
        kDexAnnotationLong = 0x06,
        kDexAnnotationFloat = 0x10,
        kDexAnnotationDouble = 0x11,
        kDexAnnotationMethodType = 0x15,
        kDexAnnotationMethodHandle = 0x16,
        kDexAnnotationString = 0x17,
        kDexAnnotationType = 0x18,
        kDexAnnotationField = 0x19,
        kDexAnnotationMethod = 0x1a,
        kDexAnnotationEnum = 0x1b,
        kDexAnnotationArray = 0x1c,
        kDexAnnotationAnnotation = 0x1d,
        kDexAnnotationNull = 0x1e,
        kDexAnnotationBoolean = 0x1f,

        kDexAnnotationValueTypeMask = 0x1f,     /* low 5 bits */
        kDexAnnotationValueArgShift = 5,
    };

    enum AnnotationResultStyle {  // private
        kAllObjects,
        kPrimitivesOrObjects,
        kAllRaw
    };

    struct AnnotationValue;

    const std::string &GetLocation() const {
        return location_;
    }

    // For DexFiles directly from .dex files, this is the checksum from the DexFile::Header.
    // For DexFiles opened from a zip files, this will be the ZipEntry CRC32 of classes.dex.
    uint32_t GetLocationChecksum() const {
        return location_checksum_;
    }

    const Header &GetHeader() const {
        return *header_;
    }

    // Decode the dex magic version
    uint32_t GetDexVersion() const {
        return GetHeader().GetVersion();
    }

    // Returns true if the byte string points to the magic value.
    virtual bool IsMagicValid() const = 0;

    // Returns true if the byte string after the magic is the correct value.
    virtual bool IsVersionValid() const = 0;

    // Returns true if the dex file supports default methods.
    virtual bool SupportsDefaultMethods() const = 0;


    struct PositionInfo {
        PositionInfo() = default;

        uint32_t address_ = 0;  // In 16-bit code units.
        uint32_t line_ = 0;  // Source code line number starting at 1.
        const char *source_file_ = nullptr;  // nullptr if the file from ClassDef still applies.
        bool prologue_end_ = false;
        bool epilogue_begin_ = false;
    };

    struct LocalInfo {
        LocalInfo() = default;

        const char *name_ = nullptr;  // E.g., list.  It can be nullptr if unknown.
        const char *descriptor_ = nullptr;  // E.g., Ljava/util/LinkedList;
        const char *signature_ = nullptr;  // E.g., java.util.LinkedList<java.lang.Integer>
        uint32_t start_address_ = 0;  // PC location where the local is first defined.
        uint32_t end_address_ = 0;  // PC location where the local is no longer defined.
        uint16_t reg_ = 0;  // Dex register which stores the values.
        bool is_live_ = false;  // Is the local defined and live.
    };

    // Callback for "new locals table entry".
    typedef void (*DexDebugNewLocalCb)(void *context, const LocalInfo &entry);


    // Debug info opcodes and constants
    enum {
        DBG_END_SEQUENCE = 0x00,
        DBG_ADVANCE_PC = 0x01,
        DBG_ADVANCE_LINE = 0x02,
        DBG_START_LOCAL = 0x03,
        DBG_START_LOCAL_EXTENDED = 0x04,
        DBG_END_LOCAL = 0x05,
        DBG_RESTART_LOCAL = 0x06,
        DBG_SET_PROLOGUE_END = 0x07,
        DBG_SET_EPILOGUE_BEGIN = 0x08,
        DBG_SET_FILE = 0x09,
        DBG_FIRST_SPECIAL = 0x0a,
        DBG_LINE_BASE = -4,
        DBG_LINE_RANGE = 15,
    };

    int GetPermissions() const;

    bool IsReadOnly() const;

    bool EnableWrite() const;

    bool DisableWrite() const;

    const uint8_t *Begin() const {
        return begin_;
    }

    size_t Size() const {
        return size_;
    }

    const uint8_t *DataBegin() const {
        return data_begin_;
    }

    size_t DataSize() const {
        return data_size_;
    }

    template<typename T>
    const T *DataPointer(size_t offset) const {
        return (offset != 0u) ? reinterpret_cast<const T *>(DataBegin() + offset) : nullptr;
    }

    // Number of bytes at the beginning of the dex file header which are skipped
    // when computing the adler32 checksum of the entire file.
    // TODO: 乱写的这里
    static constexpr uint32_t kNumNonChecksumBytes = 0x12345678;


protected:
    // First Dex format version supporting default methods.
    static const uint32_t kDefaultMethodsVersion = 37;

    // The base address of the memory mapping.
    const uint8_t *const begin_;

    // The size of the underlying memory allocation in bytes.
    const size_t size_;

    // The base address of the data section (same as Begin() for standard dex).
    const uint8_t *const data_begin_;

    // The size of the data section.
    const size_t data_size_;

    // Typically the dex file name when available, alternatively some identifying string.
    //
    // The ClassLinker will use this to match DexFiles the boot class
    // path to DexCache::GetLocation when loading from an image.
    const std::string location_;

    const uint32_t location_checksum_;

    // Points to the header section.
    const Header *const header_;

    // Points to the base of the string identifier list.
    const art::dex::StringId *const string_ids_;

    // Points to the base of the type identifier list.
    const art::dex::TypeId *const type_ids_;

    // Points to the base of the field identifier list.
    const art::dex::FieldId *const field_ids_;

    // Points to the base of the method identifier list.
    const art::dex::MethodId *const method_ids_;

    // Points to the base of the prototype identifier list.
    const art::dex::ProtoId *const proto_ids_;

    // Points to the base of the class definition list.
    const art::dex::ClassDef *const class_defs_;

    // Points to the base of the method handles list.
    const art::dex::MethodHandleItem *method_handles_;

    // Number of elements in the method handles list.
    size_t num_method_handles_;

    // Points to the base of the call sites id list.
    const art::dex::CallSiteIdItem *call_site_ids_;

    // Number of elements in the call sites list.
    size_t num_call_site_ids_;

    // Points to the base of the hiddenapi class data item_, or nullptr if the dex
    // file does not have one.
    const art::dex::HiddenapiClassData *hiddenapi_class_data_;

    // If this dex file was loaded from an oat file, oat_dex_file_ contains a
    // pointer to the OatDexFile it was loaded from. Otherwise oat_dex_file_ is
    // null.
    mutable const art::OatDexFile *oat_dex_file_;

    // Manages the underlying memory allocation.
    std::unique_ptr<art::DexFileContainer> container_;

    // If the dex file is a compact dex file. If false then the dex file is a standard dex file.
    const bool is_compact_dex_;

    // The domain this dex file belongs to for hidden API access checks.
    // It is decleared `mutable` because the domain is assigned after the DexFile
    // has been created and can be changed later by the runtime.
    mutable art::hiddenapi::Domain hiddenapi_domain_;

    friend class DexFileLoader;

    friend class DexFileVerifierTest;

    friend class OatWriter;
};


#endif //RDEX_DEXFILE_ART_10_H
