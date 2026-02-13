#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <fstream>
#include <filesystem>
#include <cstring>

// Simple compression using zlib (optional - can use miniz for header-only)
#ifdef USE_COMPRESSION
#include <zlib.h>
#endif

/*
 * ScenePackage Format (.zscene - Zero Scene)
 * 
 * Structure:
 * [Header]
 * [Resource Table]
 * [Scene Data]
 * [Resource Data Blocks...]
 * 
 * Header (32 bytes):
 *   - Magic: "ZSCN" (4 bytes)
 *   - Version: uint32 (4 bytes)
 *   - Flags: uint32 (4 bytes) - compression, encryption, etc.
 *   - ResourceCount: uint32 (4 bytes)
 *   - SceneDataOffset: uint64 (8 bytes)
 *   - SceneDataSize: uint64 (8 bytes)
 * 
 * Resource Table Entry (variable):
 *   - ResourceType: uint8 (1 byte) - Model, Texture, Script, Audio, etc.
 *   - NameLength: uint16 (2 bytes)
 *   - Name: char[NameLength]
 *   - DataOffset: uint64 (8 bytes)
 *   - DataSize: uint64 (8 bytes)
 *   - CompressedSize: uint64 (8 bytes) - 0 if uncompressed
 *   - Checksum: uint32 (4 bytes) - CRC32
 */

namespace ScenePackage {

enum class ResourceType : uint8_t {
    Unknown = 0,
    Model = 1,        // .glb, .fbx, etc.
    Texture = 2,      // .png, .jpg, .dds, etc.
    Script = 3,       // .lua, .wren, .js, etc.
    Audio = 4,        // .wav, .ogg, .mp3, etc.
    Material = 5,     // Custom material data
    Shader = 6,       // .spv, .glsl, etc.
    Animation = 7,    // Animation clips
    Prefab = 8,       // Prefab data
    NavMesh = 9,      // Navigation mesh
    Custom = 255      // User-defined
};

enum class CompressionType : uint8_t {
    None = 0,
    Deflate = 1,    // zlib/deflate
    LZ4 = 2,        // Fast compression
    Zstd = 3        // Best ratio
};

struct PackageHeader {
    char magic[4] = {'Z', 'S', 'C', 'N'};
    uint32_t version = 1;
    uint32_t flags = 0;
    uint32_t resourceCount = 0;
    uint64_t sceneDataOffset = 0;
    uint64_t sceneDataSize = 0;
    
    bool isValid() const {
        return std::memcmp(magic, "ZSCN", 4) == 0;
    }
};

struct ResourceEntry {
    ResourceType type = ResourceType::Unknown;
    std::string name;
    std::string virtualPath; // Logical path in scene (e.g., "models/tree.glb")
    uint64_t dataOffset = 0;
    uint64_t dataSize = 0;
    uint64_t compressedSize = 0; // 0 if not compressed
    uint32_t checksum = 0;
    CompressionType compression = CompressionType::None;
    
    bool isCompressed() const { return compressedSize > 0; }
};

// In-memory resource data
struct Resource {
    ResourceEntry entry;
    std::vector<uint8_t> data;
    
    bool isLoaded() const { return !data.empty(); }
    
    // Helper to save resource to disk
    bool saveToFile(const std::string& path) const {
        std::ofstream out(path, std::ios::binary);
        if (!out) return false;
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
        return out.good();
    }
};

// CRC32 checksum
inline uint32_t calculateCRC32(const uint8_t* data, size_t size) {
    static const uint32_t crcTable[256] ={

   /*-- Ugly, innit? --*/

   0x00000000L, 0x04c11db7L, 0x09823b6eL, 0x0d4326d9L,
   0x130476dcL, 0x17c56b6bL, 0x1a864db2L, 0x1e475005L,
   0x2608edb8L, 0x22c9f00fL, 0x2f8ad6d6L, 0x2b4bcb61L,
   0x350c9b64L, 0x31cd86d3L, 0x3c8ea00aL, 0x384fbdbdL,
   0x4c11db70L, 0x48d0c6c7L, 0x4593e01eL, 0x4152fda9L,
   0x5f15adacL, 0x5bd4b01bL, 0x569796c2L, 0x52568b75L,
   0x6a1936c8L, 0x6ed82b7fL, 0x639b0da6L, 0x675a1011L,
   0x791d4014L, 0x7ddc5da3L, 0x709f7b7aL, 0x745e66cdL,
   0x9823b6e0L, 0x9ce2ab57L, 0x91a18d8eL, 0x95609039L,
   0x8b27c03cL, 0x8fe6dd8bL, 0x82a5fb52L, 0x8664e6e5L,
   0xbe2b5b58L, 0xbaea46efL, 0xb7a96036L, 0xb3687d81L,
   0xad2f2d84L, 0xa9ee3033L, 0xa4ad16eaL, 0xa06c0b5dL,
   0xd4326d90L, 0xd0f37027L, 0xddb056feL, 0xd9714b49L,
   0xc7361b4cL, 0xc3f706fbL, 0xceb42022L, 0xca753d95L,
   0xf23a8028L, 0xf6fb9d9fL, 0xfbb8bb46L, 0xff79a6f1L,
   0xe13ef6f4L, 0xe5ffeb43L, 0xe8bccd9aL, 0xec7dd02dL,
   0x34867077L, 0x30476dc0L, 0x3d044b19L, 0x39c556aeL,
   0x278206abL, 0x23431b1cL, 0x2e003dc5L, 0x2ac12072L,
   0x128e9dcfL, 0x164f8078L, 0x1b0ca6a1L, 0x1fcdbb16L,
   0x018aeb13L, 0x054bf6a4L, 0x0808d07dL, 0x0cc9cdcaL,
   0x7897ab07L, 0x7c56b6b0L, 0x71159069L, 0x75d48ddeL,
   0x6b93dddbL, 0x6f52c06cL, 0x6211e6b5L, 0x66d0fb02L,
   0x5e9f46bfL, 0x5a5e5b08L, 0x571d7dd1L, 0x53dc6066L,
   0x4d9b3063L, 0x495a2dd4L, 0x44190b0dL, 0x40d816baL,
   0xaca5c697L, 0xa864db20L, 0xa527fdf9L, 0xa1e6e04eL,
   0xbfa1b04bL, 0xbb60adfcL, 0xb6238b25L, 0xb2e29692L,
   0x8aad2b2fL, 0x8e6c3698L, 0x832f1041L, 0x87ee0df6L,
   0x99a95df3L, 0x9d684044L, 0x902b669dL, 0x94ea7b2aL,
   0xe0b41de7L, 0xe4750050L, 0xe9362689L, 0xedf73b3eL,
   0xf3b06b3bL, 0xf771768cL, 0xfa325055L, 0xfef34de2L,
   0xc6bcf05fL, 0xc27dede8L, 0xcf3ecb31L, 0xcbffd686L,
   0xd5b88683L, 0xd1799b34L, 0xdc3abdedL, 0xd8fba05aL,
   0x690ce0eeL, 0x6dcdfd59L, 0x608edb80L, 0x644fc637L,
   0x7a089632L, 0x7ec98b85L, 0x738aad5cL, 0x774bb0ebL,
   0x4f040d56L, 0x4bc510e1L, 0x46863638L, 0x42472b8fL,
   0x5c007b8aL, 0x58c1663dL, 0x558240e4L, 0x51435d53L,
   0x251d3b9eL, 0x21dc2629L, 0x2c9f00f0L, 0x285e1d47L,
   0x36194d42L, 0x32d850f5L, 0x3f9b762cL, 0x3b5a6b9bL,
   0x0315d626L, 0x07d4cb91L, 0x0a97ed48L, 0x0e56f0ffL,
   0x1011a0faL, 0x14d0bd4dL, 0x19939b94L, 0x1d528623L,
   0xf12f560eL, 0xf5ee4bb9L, 0xf8ad6d60L, 0xfc6c70d7L,
   0xe22b20d2L, 0xe6ea3d65L, 0xeba91bbcL, 0xef68060bL,
   0xd727bbb6L, 0xd3e6a601L, 0xdea580d8L, 0xda649d6fL,
   0xc423cd6aL, 0xc0e2d0ddL, 0xcda1f604L, 0xc960ebb3L,
   0xbd3e8d7eL, 0xb9ff90c9L, 0xb4bcb610L, 0xb07daba7L,
   0xae3afba2L, 0xaafbe615L, 0xa7b8c0ccL, 0xa379dd7bL,
   0x9b3660c6L, 0x9ff77d71L, 0x92b45ba8L, 0x9675461fL,
   0x8832161aL, 0x8cf30badL, 0x81b02d74L, 0x857130c3L,
   0x5d8a9099L, 0x594b8d2eL, 0x5408abf7L, 0x50c9b640L,
   0x4e8ee645L, 0x4a4ffbf2L, 0x470cdd2bL, 0x43cdc09cL,
   0x7b827d21L, 0x7f436096L, 0x7200464fL, 0x76c15bf8L,
   0x68860bfdL, 0x6c47164aL, 0x61043093L, 0x65c52d24L,
   0x119b4be9L, 0x155a565eL, 0x18197087L, 0x1cd86d30L,
   0x029f3d35L, 0x065e2082L, 0x0b1d065bL, 0x0fdc1becL,
   0x3793a651L, 0x3352bbe6L, 0x3e119d3fL, 0x3ad08088L,
   0x2497d08dL, 0x2056cd3aL, 0x2d15ebe3L, 0x29d4f654L,
   0xc5a92679L, 0xc1683bceL, 0xcc2b1d17L, 0xc8ea00a0L,
   0xd6ad50a5L, 0xd26c4d12L, 0xdf2f6bcbL, 0xdbee767cL,
   0xe3a1cbc1L, 0xe760d676L, 0xea23f0afL, 0xeee2ed18L,
   0xf0a5bd1dL, 0xf464a0aaL, 0xf9278673L, 0xfde69bc4L,
   0x89b8fd09L, 0x8d79e0beL, 0x803ac667L, 0x84fbdbd0L,
   0x9abc8bd5L, 0x9e7d9662L, 0x933eb0bbL, 0x97ffad0cL,
   0xafb010b1L, 0xab710d06L, 0xa6322bdfL, 0xa2f33668L,
   0xbcb4666dL, 0xb8757bdaL, 0xb5365d03L, 0xb1f740b4L
};    
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; i++) {
        crc = crcTable[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

// Scene Package Writer
class PackageWriter {
public:
    PackageWriter() = default;
    
    // Add resource from file
    bool addResourceFromFile(const std::string& filepath, 
                            ResourceType type,
                            const std::string& virtualPath = "",
                            CompressionType compression = CompressionType::None) {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file) return false;
        
        size_t size = file.tellg();
        file.seekg(0);
        
        std::vector<uint8_t> data(size);
        file.read(reinterpret_cast<char*>(data.data()), size);
        
        std::string vpath = virtualPath.empty() ? 
            std::filesystem::path(filepath).filename().string() : virtualPath;
        
        return addResource(std::filesystem::path(filepath).filename().string(),
                          vpath, type, std::move(data), compression);
    }
    
    // Add resource from memory
    bool addResource(const std::string& name,
                    const std::string& virtualPath,
                    ResourceType type,
                    std::vector<uint8_t> data,
                    CompressionType compression = CompressionType::None) {
        Resource res;
        res.entry.name = name;
        res.entry.virtualPath = virtualPath;
        res.entry.type = type;
        res.entry.dataSize = data.size();
        res.entry.checksum = calculateCRC32(data.data(), data.size());
        res.entry.compression = compression;
        
        if (compression != CompressionType::None) {
            // Compress data
            auto compressed = compressData(data, compression);
            if (!compressed.empty()) {
                res.entry.compressedSize = compressed.size();
                res.data = std::move(compressed);
            } else {
                // Compression failed, store uncompressed
                res.entry.compression = CompressionType::None;
                res.data = std::move(data);
            }
        } else {
            res.data = std::move(data);
        }
        
        resources.push_back(std::move(res));
        return true;
    }
    
    // Set scene data (your binary scene struct)
    void setSceneData(const std::vector<uint8_t>& data) {
        sceneData = data;
    }
    
  template<typename T>
void setSceneData(const T& sceneStruct) {
    // Remove the static_assert - not all scene data needs to be trivially copyable
    sceneData.resize(sizeof(T));
    std::memcpy(sceneData.data(), &sceneStruct, sizeof(T));
}
    
    // Write package to file
    bool write(const std::string& filepath) {
        std::ofstream out(filepath, std::ios::binary);
        if (!out) return false;
        
        // Calculate offsets
        PackageHeader header;
        header.resourceCount = static_cast<uint32_t>(resources.size());
        
        size_t offset = sizeof(PackageHeader);
        
        // Calculate resource table size
        for (const auto& res : resources) {
            offset += 1 + 2 + res.entry.name.size() + 
                     2 + res.entry.virtualPath.size() +
                     8 + 8 + 8 + 4 + 1; // offsets, sizes, checksum, compression
        }
        
        // Scene data comes after resource table
        header.sceneDataOffset = offset;
        header.sceneDataSize = sceneData.size();
        offset += sceneData.size();
        
        // Assign resource offsets
        for (auto& res : resources) {
            res.entry.dataOffset = offset;
            offset += res.data.size();
        }
        
        // Write header
        out.write(reinterpret_cast<const char*>(&header), sizeof(header));
        
        // Write resource table
        for (const auto& res : resources) {
            out.write(reinterpret_cast<const char*>(&res.entry.type), 1);
            
            uint16_t nameLen = static_cast<uint16_t>(res.entry.name.size());
            out.write(reinterpret_cast<const char*>(&nameLen), 2);
            out.write(res.entry.name.data(), nameLen);
            
            uint16_t vpathLen = static_cast<uint16_t>(res.entry.virtualPath.size());
            out.write(reinterpret_cast<const char*>(&vpathLen), 2);
            out.write(res.entry.virtualPath.data(), vpathLen);
            
            out.write(reinterpret_cast<const char*>(&res.entry.dataOffset), 8);
            out.write(reinterpret_cast<const char*>(&res.entry.dataSize), 8);
            out.write(reinterpret_cast<const char*>(&res.entry.compressedSize), 8);
            out.write(reinterpret_cast<const char*>(&res.entry.checksum), 4);
            out.write(reinterpret_cast<const char*>(&res.entry.compression), 1);
        }
        
        // Write scene data
        out.write(reinterpret_cast<const char*>(sceneData.data()), sceneData.size());
        
        // Write resource data
        for (const auto& res : resources) {
            out.write(reinterpret_cast<const char*>(res.data.data()), res.data.size());
        }
        
        return out.good();
    }
    
    // Get resource count
    size_t getResourceCount() const { return resources.size(); }
    
    // Get total package size estimate
    size_t estimateSize() const {
        size_t total = sizeof(PackageHeader) + sceneData.size();
        for (const auto& res : resources) {
            total += 1 + 2 + res.entry.name.size() + 
                    2 + res.entry.virtualPath.size() + 8 + 8 + 8 + 4 + 1;
            total += res.data.size();
        }
        return total;
    }
    
private:
    std::vector<Resource> resources;
    std::vector<uint8_t> sceneData;
    
    std::vector<uint8_t> compressData(const std::vector<uint8_t>& data, 
                                     CompressionType type) {
        #ifdef USE_COMPRESSION
        if (type == CompressionType::Deflate) {
            uLongf compressedSize = compressBound(data.size());
            std::vector<uint8_t> compressed(compressedSize);
            
            int result = compress(compressed.data(), &compressedSize,
                                 data.data(), data.size());
            
            if (result == Z_OK) {
                compressed.resize(compressedSize);
                return compressed;
            }
        }
        #endif
        // Compression not available or failed
        return {};
    }
};

// Scene Package Reader
class PackageReader {
public:
    PackageReader() = default;
    
    bool open(const std::string& filepath) {
        file.open(filepath, std::ios::binary);
        if (!file) return false;
        
        // Read header
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!header.isValid()) {
            file.close();
            return false;
        }
        
        // Read resource table
        resourceEntries.clear();
        resourceEntries.reserve(header.resourceCount);
        
        for (uint32_t i = 0; i < header.resourceCount; i++) {
            ResourceEntry entry;
            
            file.read(reinterpret_cast<char*>(&entry.type), 1);
            
            uint16_t nameLen;
            file.read(reinterpret_cast<char*>(&nameLen), 2);
            entry.name.resize(nameLen);
            file.read(entry.name.data(), nameLen);
            
            uint16_t vpathLen;
            file.read(reinterpret_cast<char*>(&vpathLen), 2);
            entry.virtualPath.resize(vpathLen);
            file.read(entry.virtualPath.data(), vpathLen);
            
            file.read(reinterpret_cast<char*>(&entry.dataOffset), 8);
            file.read(reinterpret_cast<char*>(&entry.dataSize), 8);
            file.read(reinterpret_cast<char*>(&entry.compressedSize), 8);
            file.read(reinterpret_cast<char*>(&entry.checksum), 4);
            file.read(reinterpret_cast<char*>(&entry.compression), 1);
            
            resourceEntries.push_back(entry);
        }
        
        return true;
    }
    
    void close() {
        if (file.is_open()) {
            file.close();
        }
    }
    
    // Read scene data
    std::vector<uint8_t> readSceneData() {
        if (!file.is_open()) return {};
        
        file.seekg(header.sceneDataOffset);
        std::vector<uint8_t> data(header.sceneDataSize);
        file.read(reinterpret_cast<char*>(data.data()), header.sceneDataSize);
        
        return data;
    }
    
    template<typename T>
    bool readSceneData(T& sceneStruct) {
        static_assert(std::is_trivially_copyable<T>::value,
            "Scene struct must be trivially copyable");
        
        if (header.sceneDataSize != sizeof(T)) return false;
        
        file.seekg(header.sceneDataOffset);
        file.read(reinterpret_cast<char*>(&sceneStruct), sizeof(T));
        
        return file.good();
    }
    
    // Get resource list
    const std::vector<ResourceEntry>& getResourceEntries() const {
        return resourceEntries;
    }
    
    // Find resource by name
    int findResource(const std::string& name) const {
        for (size_t i = 0; i < resourceEntries.size(); i++) {
            if (resourceEntries[i].name == name) return static_cast<int>(i);
        }
        return -1;
    }
    
    // Find resource by virtual path
    int findResourceByPath(const std::string& virtualPath) const {
        for (size_t i = 0; i < resourceEntries.size(); i++) {
            if (resourceEntries[i].virtualPath == virtualPath) return static_cast<int>(i);
        }
        return -1;
    }
    
    // Read resource data
    std::vector<uint8_t> readResource(int index) {
        if (index < 0 || index >= static_cast<int>(resourceEntries.size())) {
            return {};
        }
        
        const auto& entry = resourceEntries[index];
        
        file.seekg(entry.dataOffset);
        size_t readSize = entry.isCompressed() ? entry.compressedSize : entry.dataSize;
        std::vector<uint8_t> data(readSize);
        file.read(reinterpret_cast<char*>(data.data()), readSize);
        
        // Decompress if needed
        if (entry.isCompressed()) {
            auto decompressed = decompressData(data, entry.compression, entry.dataSize);
            if (decompressed.empty()) return {};
            data = std::move(decompressed);
        }
        
        // Verify checksum
        uint32_t checksum = calculateCRC32(data.data(), data.size());
        if (checksum != entry.checksum) {
            // Checksum mismatch!
            return {};
        }
        
        return data;
    }
    
    // Read resource by name
    std::vector<uint8_t> readResource(const std::string& name) {
        int index = findResource(name);
        return index >= 0 ? readResource(index) : std::vector<uint8_t>{};
    }
    
    // Extract resource to file
    bool extractResource(int index, const std::string& outputPath) {
        auto data = readResource(index);
        if (data.empty()) return false;
        
        std::ofstream out(outputPath, std::ios::binary);
        if (!out) return false;
        
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
        return out.good();
    }
    
    // Extract all resources to directory
    bool extractAll(const std::string& outputDir) {
        std::filesystem::create_directories(outputDir);
        
        for (size_t i = 0; i < resourceEntries.size(); i++) {
            std::string outPath = outputDir + "/" + resourceEntries[i].virtualPath;
            std::filesystem::create_directories(
                std::filesystem::path(outPath).parent_path());
            
            if (!extractResource(static_cast<int>(i), outPath)) {
                return false;
            }
        }
        return true;
    }
    
    // Get info
    const PackageHeader& getHeader() const { return header; }
    size_t getResourceCount() const { return resourceEntries.size(); }
    
private:
    std::ifstream file;
    PackageHeader header;
    std::vector<ResourceEntry> resourceEntries;
    
    std::vector<uint8_t> decompressData(const std::vector<uint8_t>& compressed,
                                       CompressionType type,
                                       size_t originalSize) {
        #ifdef USE_COMPRESSION
        if (type == CompressionType::Deflate) {
            std::vector<uint8_t> decompressed(originalSize);
            uLongf destLen = originalSize;
            
            int result = uncompress(decompressed.data(), &destLen,
                                   compressed.data(), compressed.size());
            
            if (result == Z_OK) {
                return decompressed;
            }
        }
        #endif
        return {};
    }
};

} // namespace ScenePackage
