#ifndef _YES_3D_MODEL_DATA_STRUCTURES
#define _YES_3D_MODEL_DATA_STRUCTURES

#include <string>
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define MAX_BONES 100
#define NUM_BONES_PER_VERTEX 4

#define YI_FBX_REPORT_DROPPED_VERTEX_BONE_DATA 0

struct VectorKey
{
    double mTime;
    glm::vec3 mValue;
};

struct QuatKey
{
    double mTime;
    glm::quat mValue;
};

enum AnimBehaviour
{
    // The value from the default node transformation
    AnimBehaviour_DEFAULT = 0x0,

    // The nearest key value is used without interpolation
    AnimBehaviour_CONSTANT = 0x1,

    // The value of the nearest two keys is linearly extrapolated for the current time value.
    AnimBehaviour_LINEAR = 0x2,

    // The animation is repeated.
    AnimBehaviour_REPEAT = 0x3,
};

struct NodeAnim
{
    std::string mNodeName;
    uint32_t mNodeHash;

    std::vector<VectorKey> mPositionKeys;
    std::vector<QuatKey> mRotationKeys;
    std::vector<VectorKey> mScalingKeys;

    AnimBehaviour mPreState;
    AnimBehaviour mPostState;
};

struct MeshKey
{
    double mTime;
    uint32_t mValue;
};

struct MeshAnim
{
    std::string mName;
    uint32_t mNumKeys;
    std::vector<MeshKey> mKeys;
};

struct Animation
{
    std::string mName;
    double mDuration;
    double mTicksPerSecond;
    std::vector<NodeAnim> mChannels;
    std::vector<MeshAnim> mMeshChannels;
};

struct VertexDataSimple
{
    glm::vec3 position;
};

struct VertexDataPoints
{
    glm::vec3 position;
    glm::vec3 color;
};

struct VertexDataTextured
{
    glm::vec3 position;
    glm::vec2 texcoord;
};

struct VertexDataWithNormals
{
    glm::vec3 position;
    glm::vec4 normal;
    glm::vec4 texcoord;
};

struct VertexDataWithBones
{
    glm::vec3 position;
    glm::vec4 normal;
    glm::vec4 texcoord;
    glm::vec4 bones;
};

enum VertexDataType
{
    VertexDataType_Simple,
    VertexDataType_Points,
    VertexDataType_Textured,
    VertexDataType_Normals,
    VertexDataType_Bones,
};

enum PrimitiveType
{
    PrimitiveType_POINT = 0x1,
    PrimitiveType_LINE = 0x2,
    PrimitiveType_TRIANGLE = 0x4,
    PrimitiveType_POLYGON = 0x8,
};

enum TextureStorageType
{
    TextureStorageType_INTERNAL,
    TextureStorageType_INTERNAL_COMPRESSED,
    TextureStorageType_FILE,
};

enum TextureType
{
    TextureType_DIFFUSE,
    TextureType_COLOR_DIFFUSE,
    TextureType_SPECULAR,
    TextureType_COLOR_SPECULAR,
    TextureType_AMBIENT,
    TextureType_COLOR_AMBIENT,
    TextureType_EMISSIVE,
    TextureType_COLOR_EMISSIVE,
    TextureType_HEIGHT,
    TextureType_NORMALS,
    TextureType_SHININESS,
    TextureType_OPACITY,
    TextureType_DISPLACEMENT,
    TextureType_LIGHTMAP,
    TextureType_REFLECTION,
    TextureType_UNKNOWN
};

struct BoneInfo
{
    glm::mat4 BoneOffset;
    glm::mat4 FinalTransformation;

    BoneInfo()
    {
        BoneOffset = glm::mat4();
        FinalTransformation = glm::mat4();
    }
};

struct VertexBoneData
{
    struct Data
    {
        uint32_t ID;
        float Weight;
    };

    std::vector<Data> SortedData;

#define YI_FBX_REPORT_DROPPED_VERTEX_BONE_DATA 0

    VertexBoneData::VertexBoneData()
    {
        SortedData.reserve(NUM_BONES_PER_VERTEX * 2);
        Reset();
    }

    void VertexBoneData::Reset()
    {
        SortedData.clear();

        Data data = { 0, 0.0f };
        SortedData.resize(NUM_BONES_PER_VERTEX, data);
    }

    void VertexBoneData::AddBoneData(uint32_t BoneID, float Weight)
    {
        Data newData = { BoneID, Weight };

        bool bInserted = false;
        std::vector<Data>::iterator it = SortedData.begin(), end = SortedData.end();
        while (it != end)
        {
            if (Weight > (*it).Weight)
            {
                SortedData.insert(it, newData);
                bInserted = true;
                break;
            }
            ++it;
        }

        if (!bInserted)
        {
            SortedData.push_back(newData);
        }

#if YI_FBX_REPORT_DROPPED_VERTEX_BONE_DATA
        if (SortedData.size() > NUM_BONES_PER_VERTEX * 2)
        {
            YI_LOGE(LOG_TAG, "3D Model has TOO MANY VERTEX BONE DATA (%zu) affecting a single vertex", SortedData.size() - NUM_BONES_PER_VERTEX);
            for (size_t i = 0; i < SortedData.size() - NUM_BONES_PER_VERTEX; ++i)
            {
                YI_LOGE(LOG_TAG, "    BoneID = %3d, Weight = %f%s", SortedData[i].ID, SortedData[i].Weight, (i >= NUM_BONES_PER_VERTEX ? " (DROPPED!)" : ""));
            }
        }
#endif
    }

    static bool VertexBoneData::IsGreater(Data &a, Data &b)
    {
        return a.Weight > b.Weight;
    }
};

//Stu's hack for files that we don't support in formats that embed the file name. HINT: Make copies as .png first!
class CImagePreProcess
{
public:
    static bool ReplaceUnrecognizedInternalTextureFormats(std::string &sFilename)
    {
        std::string sTest = sFilename;
        std::transform(sTest.begin(), sTest.end(), sTest.begin(), ::tolower);
        std::string sTif = ".tif";
        size_t start_pos = sTest.find(sTif);
        if (start_pos != std::string::npos)
        {
            sFilename.replace(start_pos, sTif.length(), ".png");
            return true;
        }
        std::string sPSD = ".psd";
        start_pos = sTest.find(sPSD);
        if (start_pos != std::string::npos)
        {
            sFilename.replace(start_pos, sPSD.length(), ".png");
            return true;
        }
        return false;
    }
};

#endif // _YES_3D_MODEL_DATA_STRUCTURES
