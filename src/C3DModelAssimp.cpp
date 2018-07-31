#include "C3DModelAssimp.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/cimport.h>

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <climits>

#define STU_EXPORT_SEQUENTIAL 1 //When enabled we write to the file at each model (much better memory usage, but may be slightly slower)

#define WRITE_VALUE(x)          Data.insert(Data.end(), (uint8_t *)&(x), (uint8_t *)&(x) + sizeof(x)); uSize += sizeof(x);
#define WRITE_VALUES(x, c)      Data.insert(Data.end(), (uint8_t *)&(x), (uint8_t *)&(x) + sizeof(x) * (c)); uSize += sizeof(x) * (c);
#define WRITE_BYTES(x, y, c)    Data.insert(Data.end(), (uint8_t *)(x), (uint8_t *)(x) + sizeof(y) * (c)); uSize += sizeof(y) * (c);

#define LOG_ERROR(...) printf("C3DModelAssimp:"); printf(__VA_ARGS__);
#define LOG_INFO(...) printf("C3DModelAssimp:"); printf(__VA_ARGS__);

static const std::string LOG_TAG("C3DModelAssimp");

glm::mat4 CopyMatrixAssimpToGL(aiMatrix4x4 m)
{
    // OpenGL matrices are column major
    m.Transpose();

    glm::mat4 matrix(m.a1, m.a2, m.a3, m.a4,
                     m.b1, m.b2, m.b3, m.b4,
                     m.c1, m.c2, m.c3, m.c4,
                     m.d1, m.d2, m.d3, m.d4);
    return matrix;
}

glm::mat4 CopyMatrix3x3AssimpToGL(aiMatrix3x3 m)
{
    // OpenGL matrices are column major
    m.Transpose();

    glm::mat4 matrix(m.a1, m.a2, m.a3, 0,
                     m.b1, m.b2, m.b3, 0,
                     m.c1, m.c2, m.c3, 0,
                     0, 0, 0, 1);
    return matrix;
}

C3DModelAssimp::C3DModelAssimp()
    : m_pAIScene(YI_NULL),
    m_bFlipUVonY(false),
    m_uNumBones(0),
    m_bHasAnimations(false)
{
    // Change this line to normal if you not want to analyse the import process
    //Assimp::Logger::LogSeverity severity = Assimp::Logger::NORMAL;
    Assimp::Logger::LogSeverity severity = Assimp::Logger::VERBOSE;

    // Create a logger instance for Console Output
    Assimp::DefaultLogger::create("", severity, aiDefaultLogStream_STDOUT);

    // Create a logger instance for File Output (found in project folder or near .exe)
    Assimp::DefaultLogger::create("assimp_log.txt", severity, aiDefaultLogStream_FILE);

    // Now I am ready for logging my stuff
    Assimp::DefaultLogger::get()->info("this is my info-call");
}

C3DModelAssimp::~C3DModelAssimp()
{
    // Kill it after the work is done
    Assimp::DefaultLogger::kill();
}

bool C3DModelAssimp::ImportAssimp(const std::string &path, bool bFlipUV)
{
    LOG_INFO("Attempting to load '%s' 3D model.", path.c_str());

    // TODO we support Texture UVCords transforms, we might be able to get rid of aiProcess_TransformUVCoords at a later time...
    uint32_t uFlags = aiProcessPreset_TargetRealtime_Quality | aiProcess_TransformUVCoords;

    if (CFileExportSTUFormat::EndsWithIgnoreCase(path, ".x"))
    {
        // For DirectX X format, we need to convert to Left handed coordinate.
        uFlags |= aiProcess_ConvertToLeftHanded;
    }

    if (CFileExportSTUFormat::EndsWithIgnoreCase(path, ".pk3"))
    {
        uFlags |= aiProcess_MakeLeftHanded | aiProcess_FlipUVs;
    }
    uFlags |= aiProcess_Triangulate;

    //Point clouds should be just loaded.
    if (CFileExportSTUFormat::EndsWithIgnoreCase(path, ".ply"))
    {
        uFlags = aiProcess_SplitLargeMeshes;
    }
    m_importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, USHRT_MAX - 1);
    m_importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, USHRT_MAX - 1);
    const aiScene* pLayout = m_importer.ReadFile(path, uFlags);

    if (!pLayout)
    {
        LOG_ERROR("Could not load '%s' model: %s", path.c_str(), m_importer.GetErrorString());
        return false;
    }

    LOG_INFO("Successfully imported '%s' 3D model.", path.c_str());

    m_pAIScene = m_importer.GetScene();

    if (m_pAIScene)
    {
        return true;
    }
    return false;
}

bool C3DModelAssimp::ExportToSTUFormat(const std::string &path)
{
    return ExportToSTUFormat(path, true);
}

bool C3DModelAssimp::ExportToSTUFormat(const std::string &path, bool bFlipUV)
{
    uint32_t NumVertices = 0;
    bool bHasBones = false;

    m_sSTUPath = path;
    m_sSTUPath.append(".stu");
    std::remove(m_sSTUPath.c_str());

    m_bFlipUVonY = bFlipUV;
    m_Entries.clear();
    m_Bones.clear();
    m_TotalMeshCount = 0;

    if (!ImportAssimp(path, bFlipUV))
    {
        return false;
    }

    if (m_pAIScene->mNumMeshes == 0)
    {
        return false;
    }

    m_Entries.resize(m_pAIScene->mNumMeshes);

    // Count the number of vertices to allocate for possible bones.
    for (uint32_t i = 0; i < m_Entries.size(); i++)
    {
        m_Entries[i].uBaseVertex = NumVertices;
        NumVertices += m_pAIScene->mMeshes[i]->mNumVertices;
        if (m_pAIScene->mMeshes[i]->HasBones())
        {
            bHasBones = true;
        }
    }
    if (bHasBones)
    {
        m_Bones.resize(NumVertices);
    }

    if (m_pAIScene->HasAnimations())
    {
        m_VertexDataType = VertexDataType_Bones;
        m_bHasAnimations = true;
        ExportAnimations();
    }

    ExportTextures();
    ExportSceneTree();

    m_Bones.resize(0);    //No longer required
    m_Bones.clear();
    if (bHasBones)
    {
        ExportBones();
    }

#if !STU_EXPORT_SEQUENTIAL
    m_Export.ExportFile(m_sSTUPath);
#endif

    return true;
}

void C3DModelAssimp::ExportAnimations()
{
    uint32_t uSize = 0;
    uint8_t bytes[128] = { 0 };
    std::vector< uint8_t > Data;

    WRITE_VALUE(m_pAIScene->mNumAnimations);

    for (uint32_t i = 0; i < m_pAIScene->mNumAnimations; i++)
    {
        uSize += CFileExportSTUFormat::CopyString(m_pAIScene->mAnimations[i]->mName.C_Str(), &Data);

        WRITE_VALUE(m_pAIScene->mAnimations[i]->mTicksPerSecond);
        WRITE_VALUE(m_pAIScene->mAnimations[i]->mDuration);
        WRITE_VALUE(m_pAIScene->mAnimations[i]->mNumChannels);

        for (uint32_t j = 0; j < m_pAIScene->mAnimations[i]->mNumChannels; j++)
        {
            uSize += CFileExportSTUFormat::CopyString(m_pAIScene->mAnimations[i]->mChannels[j]->mNodeName.C_Str(), &Data);

            WRITE_VALUE(m_pAIScene->mAnimations[i]->mChannels[j]->mNumPositionKeys);
            WRITE_VALUE(m_pAIScene->mAnimations[i]->mChannels[j]->mNumRotationKeys);
            WRITE_VALUE(m_pAIScene->mAnimations[i]->mChannels[j]->mNumScalingKeys);
            WRITE_VALUE(m_pAIScene->mAnimations[i]->mChannels[j]->mPostState);
            WRITE_VALUE(m_pAIScene->mAnimations[i]->mChannels[j]->mPreState);

            for (uint32_t k = 0; k < m_pAIScene->mAnimations[i]->mChannels[j]->mNumPositionKeys; k++)
            {
                VectorKey key;
                key.mTime = m_pAIScene->mAnimations[i]->mChannels[j]->mPositionKeys[k].mTime;
                key.mValue = glm::vec3(m_pAIScene->mAnimations[i]->mChannels[j]->mPositionKeys[k].mValue.x, m_pAIScene->mAnimations[i]->mChannels[j]->mPositionKeys[k].mValue.y, m_pAIScene->mAnimations[i]->mChannels[j]->mPositionKeys[k].mValue.z);
                Data.insert(Data.end(), (uint8_t *)&key, (uint8_t *)&key + sizeof(key));
                uSize += sizeof(key);
            }
            for (uint32_t k = 0; k < m_pAIScene->mAnimations[i]->mChannels[j]->mNumRotationKeys; k++)
            {
                QuatKey key;
                key.mTime = m_pAIScene->mAnimations[i]->mChannels[j]->mRotationKeys[k].mTime;
                key.mValue = glm::quat(m_pAIScene->mAnimations[i]->mChannels[j]->mRotationKeys[k].mValue.w, m_pAIScene->mAnimations[i]->mChannels[j]->mRotationKeys[k].mValue.x, m_pAIScene->mAnimations[i]->mChannels[j]->mRotationKeys[k].mValue.y, m_pAIScene->mAnimations[i]->mChannels[j]->mRotationKeys[k].mValue.z);
                Data.insert(Data.end(), (uint8_t *)&key, (uint8_t *)&key + sizeof(key));
                uSize += sizeof(key);
            }
            for (uint32_t k = 0; k < m_pAIScene->mAnimations[i]->mChannels[j]->mNumScalingKeys; k++)
            {
                VectorKey key;
                key.mTime = m_pAIScene->mAnimations[i]->mChannels[j]->mScalingKeys[k].mTime;
                key.mValue = glm::vec3(m_pAIScene->mAnimations[i]->mChannels[j]->mScalingKeys[k].mValue.x, m_pAIScene->mAnimations[i]->mChannels[j]->mScalingKeys[k].mValue.y, m_pAIScene->mAnimations[i]->mChannels[j]->mScalingKeys[k].mValue.z);
                Data.insert(Data.end(), (uint8_t *)&key, (uint8_t *)&key + sizeof(key));
                uSize += sizeof(key);
            }
        }

        WRITE_VALUE(m_pAIScene->mAnimations[i]->mNumMeshChannels);

        for (uint32_t j = 0; j < m_pAIScene->mAnimations[i]->mNumMeshChannels; j++)
        {
            uSize += CFileExportSTUFormat::CopyString(m_pAIScene->mAnimations[i]->mMeshChannels[j]->mName.C_Str(), &Data);

            WRITE_VALUE(m_pAIScene->mAnimations[i]->mMeshChannels[j]->mNumKeys);

            for (uint32_t k = 0; k < m_pAIScene->mAnimations[i]->mMeshChannels[j]->mNumKeys; k++)
            {
                MeshKey key;
                key.mTime = m_pAIScene->mAnimations[i]->mMeshChannels[j]->mKeys[k].mTime;
                key.mValue = m_pAIScene->mAnimations[i]->mMeshChannels[j]->mKeys[k].mValue;
                Data.insert(Data.end(), (uint8_t *)&key, (uint8_t *)&key + sizeof(key));
                uSize += sizeof(key);
            }
        }
    }
#if STU_EXPORT_SEQUENTIAL
    m_Export.AppendChunkToFile(m_sSTUPath, "Animations", &Data.at(0), uSize);
#else
    m_Export.WriteChunk("Animations", &Data.at(0), uSize);
#endif
}

void C3DModelAssimp::ExportBones()
{
    uint32_t uSize = 0;
    uint8_t bytes[128] = { 0 };
    uint32_t uValue;
    std::vector< uint8_t > Data;

    uValue = (int32_t)m_BoneMapping.size();
    WRITE_VALUE(uValue);

    std::map<uint32_t, uint32_t> ::iterator Itr = m_BoneMapping.begin();
    std::map<uint32_t, uint32_t> ::iterator End = m_BoneMapping.end();
    while (Itr != End)
    {
        WRITE_VALUE(Itr->first);
        WRITE_VALUE(Itr->second);

        Itr++;
    }
    uValue = (int32_t)m_BoneInfo.size();
    WRITE_VALUE(uValue);

    if (uValue)
    {
        WRITE_VALUES(m_BoneInfo[0], (int32_t)m_BoneInfo.size());
    }

#if STU_EXPORT_SEQUENTIAL
    m_Export.AppendChunkToFile(m_sSTUPath, "Bones", &Data.at(0), uSize);
#else
    m_Export.WriteChunk("Bones", &Data.at(0), uSize);
#endif
}

void C3DModelAssimp::ExportTextures()
{
    uint32_t uValue;
    uint32_t uSize = 0;
    uint8_t bytes[128] = { 0 };
    std::vector< uint8_t > Data;

    uValue = m_pAIScene->mNumTextures;
    WRITE_VALUE(uValue);

    for (uint32_t i = 0; i < m_pAIScene->mNumTextures; i++)
    {
        const aiTexture& als = *m_pAIScene->mTextures[i];

        std::string embeddedTextureName = "InternalTexture(" + std::to_string(i) + ")";

        if (als.mHeight == 0) // Compressed texture.
        {
            uValue = TextureStorageType_INTERNAL_COMPRESSED;
            WRITE_VALUE(uValue);

            uSize += CFileExportSTUFormat::CopyString(embeddedTextureName.c_str(), &Data);

            WRITE_VALUE(als.mWidth);
            WRITE_BYTES(als.pcData, uint8_t, als.mWidth);
        }
        else
        {
            uValue = TextureStorageType_INTERNAL;
            WRITE_VALUE(uValue);

            uSize += CFileExportSTUFormat::CopyString(embeddedTextureName.c_str(), &Data);

            WRITE_VALUE(als.mWidth);
            WRITE_VALUE(als.mHeight);

            if (als.CheckFormat("rgba8888"))
            {
                WRITE_VALUES(*als.pcData, als.mWidth * als.mHeight);
            }
            else if (als.CheckFormat("rgba8880"))
            {
                uint32_t * pBitmap = new uint32_t[als.mWidth * als.mHeight];
                uint8_t *pTexel = (uint8_t*)als.pcData;
                for (uint32_t Tex = 0; Tex < als.mWidth * als.mHeight; ++Tex)
                {
                    uint32_t color = (pTexel[Tex]) | (pTexel[Tex + 1] << 8) | (pTexel[Tex + 2] << 16) | (0xFF << 24);
                    pBitmap[Tex] = color;
                }
                WRITE_VALUES(pBitmap, als.mWidth * als.mHeight);
                delete pBitmap;
            }
        }
    }
#if STU_EXPORT_SEQUENTIAL
    m_Export.AppendChunkToFile(m_sSTUPath, "Textures", &Data.at(0), uSize);
#else
    m_Export.WriteChunk("Textures", &Data.at(0), uSize);
#endif
}

void C3DModelAssimp::ExportSceneTree()
{
    uint32_t uIndex = 0;
    m_uSubModelCount = 0;
    m_uSubModelVertexCount = 0;

    ExportSubTree(m_pAIScene->mRootNode, uIndex);
}

void C3DModelAssimp::WriteVertexChunk(std::string sName, uint32_t uCurrentMesh, const aiMesh *pLayoutMesh, VertexDataType m_VertexDataType)
{
    uint32_t uSize = 0;
    uint8_t bytes[128] = { 0 };
    std::vector< uint8_t > Data;
    VertexDataWithNormals Vertex;
    float bmin[3], bmax[3];

    bmin[0] = bmin[1] = bmin[2] = std::numeric_limits<float>::max();
    bmax[0] = bmax[1] = bmax[2] = -std::numeric_limits<float>::max();

    switch (m_VertexDataType)
    {
    case VertexDataType_Simple:
    {
        Data.reserve(sizeof(VertexDataSimple) * pLayoutMesh->mNumVertices);
        if (Data.capacity() < (sizeof(VertexDataSimple) * pLayoutMesh->mNumVertices))
        {
            LOG_ERROR("Ran out of memory while exporting vertices from '%s' model.", m_sSTUPath.c_str());
            return;
        }
        VertexDataSimple Vertex;
        for (uint32_t vertId = 0; vertId < pLayoutMesh->mNumVertices; ++vertId)
        {
            Vertex.position.x = pLayoutMesh->mVertices[vertId].x;
            Vertex.position.y = pLayoutMesh->mVertices[vertId].y;
            Vertex.position.z = pLayoutMesh->mVertices[vertId].z;
            bmin[0] = std::min(Vertex.position.x, bmin[0]);
            bmin[1] = std::min(Vertex.position.y, bmin[1]);
            bmin[2] = std::min(Vertex.position.z, bmin[2]);
            bmax[0] = std::max(Vertex.position.x, bmax[0]);
            bmax[1] = std::max(Vertex.position.y, bmax[1]);
            bmax[2] = std::max(Vertex.position.z, bmax[2]);
            WRITE_VALUE(Vertex);
        }
        break;
    }
    case VertexDataType_Points:
    {
        Data.reserve(sizeof(VertexDataPoints) * pLayoutMesh->mNumVertices);
        if (Data.capacity() < (sizeof(VertexDataPoints) * pLayoutMesh->mNumVertices))
        {
            LOG_ERROR("Ran out of memory while exporting vertices from '%s' model.", m_sSTUPath.c_str());
            return;
        }
        VertexDataPoints Vertex;
        for (uint32_t vertId = 0; vertId < pLayoutMesh->mNumVertices; ++vertId)
        {
            Vertex.position.x = pLayoutMesh->mVertices[vertId].x;
            Vertex.position.y = pLayoutMesh->mVertices[vertId].y;
            Vertex.position.z = pLayoutMesh->mVertices[vertId].z;
            bmin[0] = std::min(Vertex.position.x, bmin[0]);
            bmin[1] = std::min(Vertex.position.y, bmin[1]);
            bmin[2] = std::min(Vertex.position.z, bmin[2]);
            bmax[0] = std::max(Vertex.position.x, bmax[0]);
            bmax[1] = std::max(Vertex.position.y, bmax[1]);
            bmax[2] = std::max(Vertex.position.z, bmax[2]);
            Vertex.color.r = pLayoutMesh->mColors[0][vertId].r;
            Vertex.color.g = pLayoutMesh->mColors[0][vertId].g;
            Vertex.color.b = pLayoutMesh->mColors[0][vertId].b;
            WRITE_VALUE(Vertex);
        }
        break;
    }
    case VertexDataType_Textured:
    {
        Data.reserve(sizeof(VertexDataTextured) * pLayoutMesh->mNumVertices);
        if (Data.capacity() < (sizeof(VertexDataTextured) * pLayoutMesh->mNumVertices))
        {
            LOG_ERROR("Ran out of memory while exporting vertices from '%s' model.", m_sSTUPath.c_str());
            return;
        }
        VertexDataTextured Vertex;
        for (uint32_t vertId = 0; vertId < pLayoutMesh->mNumVertices; ++vertId)
        {
            Vertex.position.x = pLayoutMesh->mVertices[vertId].x;
            Vertex.position.y = pLayoutMesh->mVertices[vertId].y;
            Vertex.position.z = pLayoutMesh->mVertices[vertId].z;
            bmin[0] = std::min(Vertex.position.x, bmin[0]);
            bmin[1] = std::min(Vertex.position.y, bmin[1]);
            bmin[2] = std::min(Vertex.position.z, bmin[2]);
            bmax[0] = std::max(Vertex.position.x, bmax[0]);
            bmax[1] = std::max(Vertex.position.y, bmax[1]);
            bmax[2] = std::max(Vertex.position.z, bmax[2]);
            Vertex.texcoord.x = pLayoutMesh->mTextureCoords[0][vertId].x;
            Vertex.texcoord.y = m_bFlipUVonY ? 1.0f - pLayoutMesh->mTextureCoords[0][vertId].y : pLayoutMesh->mTextureCoords[0][vertId].y;
            WRITE_VALUE(Vertex);
        }
        break;
    }
    case VertexDataType_Normals:
    {
        Data.reserve(sizeof(VertexDataWithNormals) * pLayoutMesh->mNumVertices);
        if (Data.capacity() < (sizeof(VertexDataWithNormals) * pLayoutMesh->mNumVertices))
        {
            LOG_ERROR("Ran out of memory while exporting vertices from '%s' model.", m_sSTUPath.c_str());
            return;
        }
        VertexDataWithNormals Vertex;
        for (uint32_t vertId = 0; vertId < pLayoutMesh->mNumVertices; ++vertId)
        {
            Vertex.position.x = pLayoutMesh->mVertices[vertId].x;
            Vertex.position.y = pLayoutMesh->mVertices[vertId].y;
            Vertex.position.z = pLayoutMesh->mVertices[vertId].z;
            bmin[0] = std::min(Vertex.position.x, bmin[0]);
            bmin[1] = std::min(Vertex.position.y, bmin[1]);
            bmin[2] = std::min(Vertex.position.z, bmin[2]);
            bmax[0] = std::max(Vertex.position.x, bmax[0]);
            bmax[1] = std::max(Vertex.position.y, bmax[1]);
            bmax[2] = std::max(Vertex.position.z, bmax[2]);
            Vertex.normal.x = pLayoutMesh->mNormals[vertId].x;
            Vertex.normal.y = pLayoutMesh->mNormals[vertId].y;
            Vertex.normal.z = pLayoutMesh->mNormals[vertId].z;
            if (pLayoutMesh->HasTextureCoords(0))
            {
                // texture uv + packed tangent
                glm::vec3 tc = glm::vec3(pLayoutMesh->mTextureCoords[0][vertId].x, pLayoutMesh->mTextureCoords[0][vertId].y, pLayoutMesh->mTextureCoords[0][vertId].z);
                Vertex.texcoord.x = pLayoutMesh->mTextureCoords[0][vertId].x;
                Vertex.texcoord.y = m_bFlipUVonY ? 1.0f - pLayoutMesh->mTextureCoords[0][vertId].y : pLayoutMesh->mTextureCoords[0][vertId].y;
                if (pLayoutMesh->HasTangentsAndBitangents())
                {
                    float tx = (float)(int32_t)((pLayoutMesh->mTangents[vertId].x*0.5f + 0.5f)*255.0f);
                    float ty = (float)(int32_t)int((pLayoutMesh->mTangents[vertId].y*0.5f + 0.5f)*255.0f);
                    float tz = (float)(int32_t)int((pLayoutMesh->mTangents[vertId].z*0.5f + 0.5f)*255.0f);
                    aiVector3D& n = pLayoutMesh->mNormals[vertId];
                    aiVector3D& t = pLayoutMesh->mTangents[vertId];
                    aiVector3D& b = pLayoutMesh->mBitangents[vertId];
                    float handedness = (n ^ t) * b < 0.0f ? -1.0f : 1.0f;
                    // (n ^ t) * b = dot(cross(n, t), b)
                    tc.z = (tx + ty / 256.0f + tz / 65536.0f) * handedness;
                    Vertex.texcoord.z = tc.z;
                }
            }
            else
            {
                Vertex.texcoord.x = 0.0f;
                Vertex.texcoord.y = 0.0f;
            }
            WRITE_VALUE(Vertex);
        }
        break;
    }

    case VertexDataType_Bones:
    {
        Data.reserve(sizeof(VertexDataWithBones) * pLayoutMesh->mNumVertices);
        if (Data.capacity() < (sizeof(VertexDataWithBones) * pLayoutMesh->mNumVertices))
        {
            LOG_ERROR("Ran out of memory while exporting vertices from '%s' model.", m_sSTUPath.c_str());
            return;
        }
        VertexDataWithBones Vertex;

        for (uint32_t vertId = 0; vertId < pLayoutMesh->mNumVertices; ++vertId)
        {
            Vertex.position.x = pLayoutMesh->mVertices[vertId].x;
            Vertex.position.y = pLayoutMesh->mVertices[vertId].y;
            Vertex.position.z = pLayoutMesh->mVertices[vertId].z;
            bmin[0] = std::min(Vertex.position.x, bmin[0]);
            bmin[1] = std::min(Vertex.position.y, bmin[1]);
            bmin[2] = std::min(Vertex.position.z, bmin[2]);
            bmax[0] = std::max(Vertex.position.x, bmax[0]);
            bmax[1] = std::max(Vertex.position.y, bmax[1]);
            bmax[2] = std::max(Vertex.position.z, bmax[2]);
            if (pLayoutMesh->HasNormals())
            {
                Vertex.normal.x = pLayoutMesh->mNormals[vertId].x;
                Vertex.normal.y = pLayoutMesh->mNormals[vertId].y;
                Vertex.normal.z = pLayoutMesh->mNormals[vertId].z;
            }
            else
            {
                Vertex.normal.x = 0;
                Vertex.normal.y = 0;
                Vertex.normal.z = 0;
            }
            if (pLayoutMesh->HasTextureCoords(0))
            {
                // texture uv + packed tangent
                glm::vec3 tc = glm::vec3(pLayoutMesh->mTextureCoords[0][vertId].x, pLayoutMesh->mTextureCoords[0][vertId].y, pLayoutMesh->mTextureCoords[0][vertId].z);
                Vertex.texcoord.x = pLayoutMesh->mTextureCoords[0][vertId].x;
                Vertex.texcoord.y = m_bFlipUVonY ? 1.0f - pLayoutMesh->mTextureCoords[0][vertId].y : pLayoutMesh->mTextureCoords[0][vertId].y;
                if (pLayoutMesh->HasTangentsAndBitangents())
                {
                    float tx = (float)(int32_t)((pLayoutMesh->mTangents[vertId].x*0.5f + 0.5f)*255.0f);
                    float ty = (float)(int32_t)int((pLayoutMesh->mTangents[vertId].y*0.5f + 0.5f)*255.0f);
                    float tz = (float)(int32_t)int((pLayoutMesh->mTangents[vertId].z*0.5f + 0.5f)*255.0f);
                    aiVector3D& n = pLayoutMesh->mNormals[vertId];
                    aiVector3D& t = pLayoutMesh->mTangents[vertId];
                    aiVector3D& b = pLayoutMesh->mBitangents[vertId];
                    float handedness = (n ^ t) * b < 0.0f ? -1.0f : 1.0f;
                    // (n ^ t) * b = dot(cross(n, t), b)
                    tc.z = (tx + ty / 256.0f + tz / 65536.0f) * handedness;
                    Vertex.texcoord.z = tc.z;
                }
            }
            else
            {
                Vertex.texcoord.x = 0.0f;
                Vertex.texcoord.y = 0.0f;
            }
            // skinning
            float packedboneids1 = 0.0;
            float packedboneids2 = 0.0;
            float boneWeight0 = 1.0f;
            float boneWeight1 = 0;
            float boneWeight2 = 0;
            float boneWeight3 = 0;
            if (pLayoutMesh->HasBones())
            {
            uint32_t uRealVertex = vertId + m_Entries[m_TotalMeshCount + uCurrentMesh].uBaseVertex;
                boneWeight0 = m_Bones[uRealVertex].SortedData[0].Weight;
                boneWeight1 = m_Bones[uRealVertex].SortedData[1].Weight;
                boneWeight2 = m_Bones[uRealVertex].SortedData[2].Weight;
                boneWeight3 = m_Bones[uRealVertex].SortedData[3].Weight;

                packedboneids1 = float(m_Bones[uRealVertex].SortedData[0].ID) +
                    float(m_Bones[uRealVertex].SortedData[1].ID) / 256.0f;    //256 allows for up to 200+ bones matrices before clashing.
                packedboneids2 = float(m_Bones[uRealVertex].SortedData[2].ID) +
                    float(m_Bones[uRealVertex].SortedData[3].ID) / 256.0f;
            }
            Vertex.bones.x = boneWeight0;
            Vertex.bones.y = boneWeight1;
            Vertex.bones.z = boneWeight2;
            Vertex.bones.w = boneWeight3;
            Vertex.texcoord.w = packedboneids1;
            Vertex.normal.w = packedboneids2;
            WRITE_VALUE(Vertex);
        }
        break;
    }
    }
#if STU_EXPORT_SEQUENTIAL
    m_Export.AppendChunkToFile(m_sSTUPath, sName, &Data.at(0), (int32_t)Data.size());
#else
    m_Export.WriteChunk(sChunkname, &Data.at(0), (int32_t)Data.size());
#endif
    std::vector< uint8_t >().swap(Data);

    //Write BBox chunk:
    WRITE_VALUE(bmin[0]);
    WRITE_VALUE(bmin[1]);
    WRITE_VALUE(bmin[2]);
    WRITE_VALUE(bmax[0]);
    WRITE_VALUE(bmax[1]);
    WRITE_VALUE(bmax[2]);

#if STU_EXPORT_SEQUENTIAL
    m_Export.AppendChunkToFile(m_sSTUPath, sName + "BB", &Data.at(0), (int32_t)Data.size());
#else
    Export.WriteChunk(sName + "BB", &Data.at(0), (int32_t)Data.size());
#endif
    std::vector< uint8_t >().swap(Data);
}

void C3DModelAssimp::ExportSubTree(const aiNode* pLayoutNode, uint32_t uIndex)
{
    uint32_t uSize = 0;
    uint32_t uValue;
    uint8_t bytes[128] = { 0 };
    std::vector< uint8_t > Data;

    std::string nodeName;
    static uint32_t sUniqueAssimpUnknownID = 0;
    if (strlen(pLayoutNode->mName.C_Str()) > 0)
    {
        nodeName = std::string("assimp.(") + pLayoutNode->mName.C_Str() + "-" + std::to_string(sUniqueAssimpUnknownID++) + ")";
    }
    else
    {
        nodeName = std::string("assimp.(UNKNOWN-") + std::to_string(sUniqueAssimpUnknownID++) + ")";
    }
    LOG_INFO("Found node '%s'\n", nodeName.c_str());

    aiMatrix4x4 m;
    glm::mat4 matrix;

    WRITE_VALUE(pLayoutNode->mNumChildren);

    uSize += CFileExportSTUFormat::CopyString(nodeName.c_str(), &Data);

    uSize += CFileExportSTUFormat::CopyString(pLayoutNode->mName.C_Str(), &Data);

    // Get node transformation matrix
    m = pLayoutNode->mTransformation;
    matrix = CopyMatrixAssimpToGL(m);

    WRITE_VALUE(matrix);

    WRITE_VALUE(pLayoutNode->mNumMeshes);

    // You.i engine does not support multi-mesh, so for now, let's create a child node per mesh.
    for (uint32_t i = 0; i < pLayoutNode->mNumMeshes; ++i)
    {
        const aiMesh *pLayoutMesh = m_pAIScene->mMeshes[pLayoutNode->mMeshes[i]];
        const aiMaterial *pLayoutMaterial = m_pAIScene->mMaterials[pLayoutMesh->mMaterialIndex];

        std::string meshName;
        static uint32_t sUniqueAssimpUnknownMeshID = 0;
        if (strlen(pLayoutMesh->mName.C_Str()) > 0)
        {
            meshName = nodeName + ".mesh(" + pLayoutMesh->mName.C_Str() + "-" + std::to_string(sUniqueAssimpUnknownMeshID++) + ").id(" + std::to_string(i) + ")";
        }
        else
        {
            meshName = nodeName + ".mesh(UNKNOWN-" + std::to_string(sUniqueAssimpUnknownMeshID++) + ").id(" + std::to_string(i) + ")";
        }
        LOG_INFO("Found mesh '%s'\n", meshName.c_str());
        std::string VBOName = meshName + ".VBO";
        std::string IBOName = meshName + ".IBO";

        uSize += CFileExportSTUFormat::CopyString(meshName.c_str(), &Data);

        uSize += CFileExportSTUFormat::CopyString(pLayoutMesh->mName.C_Str(), &Data);

        //LoadBones
        for (uint32_t k = 0; k < pLayoutMesh->mNumBones; k++)
        {
            uint32_t BoneIndex = 0;
            std::string BoneName(pLayoutMesh->mBones[k]->mName.data);
            uint32_t BoneHash = CFileExportSTUFormat::MakeHashFromName(BoneName);

            if (m_BoneMapping.find(BoneHash) == m_BoneMapping.end())
            {
                BoneIndex = m_uNumBones;
                m_uNumBones++;
                if (m_uNumBones > MAX_BONES)
                {
                    LOG_ERROR("3D object Exceeded maximum bone count");
                    //m_uNumBones = MAX_BONES; //Can be capped here, or passes throuugh
                }
                BoneInfo bi;
                m_BoneInfo.push_back(bi);
                m_BoneMapping[BoneHash] = BoneIndex;
                m_BoneInfo[BoneIndex].BoneOffset = CopyMatrixAssimpToGL(pLayoutMesh->mBones[k]->mOffsetMatrix);
            }
            else
            {
                BoneIndex = m_BoneMapping[BoneHash];
            }

            for (uint32_t j = 0; j < pLayoutMesh->mBones[k]->mNumWeights; j++)
            {
                uint32_t VertexID = m_Entries[m_TotalMeshCount + i].uBaseVertex + pLayoutMesh->mBones[k]->mWeights[j].mVertexId;
                float Weight = pLayoutMesh->mBones[k]->mWeights[j].mWeight;
                m_Bones[VertexID].AddBoneData(BoneIndex, Weight);
            }
        }
        if (m_bHasAnimations)
        {
            uValue = 1;
        }
        else
        {
            uValue = 0;
        }
        WRITE_VALUE(uValue);

        uValue = pLayoutMesh->mNumVertices;
        WRITE_VALUE(uValue);

        if (pLayoutMesh->mNumVertices > 0)
        {
            if (pLayoutMesh->HasNormals())
            {
                if (m_VertexDataType != VertexDataType_Bones)
                {
                    if (pLayoutMesh->HasTextureCoords(0))
                    {
                        m_VertexDataType = VertexDataType_Normals;
                    }
                    else if (pLayoutMesh->HasVertexColors(0))
                    {
                        m_VertexDataType = VertexDataType_Points;
                    }
                    else
                    {
                        m_VertexDataType = VertexDataType_Simple;
                    }
                }
        }
            else
            {
                if (pLayoutMesh->HasTextureCoords(0))
                {
                    m_VertexDataType = VertexDataType_Textured;
                }
                else if (pLayoutMesh->HasVertexColors(0))
                {
                    m_VertexDataType = VertexDataType_Points;
                }
                else
                {
                    m_VertexDataType = VertexDataType_Simple;
                }
            }
            WRITE_VALUE(m_VertexDataType);

            std::string sVertexChunkname = std::string("Vx:") + std::to_string(m_uSubModelVertexCount ++);
            WriteVertexChunk(sVertexChunkname, i, pLayoutMesh, m_VertexDataType);
        }

        std::vector<uint16_t> indices;

        if (pLayoutMesh->mPrimitiveTypes != aiPrimitiveType_POINT)
        {
            for (uint32_t faceID = 0; faceID < pLayoutMesh->mNumFaces; ++faceID)
            {
                const aiFace *pFace = &pLayoutMesh->mFaces[faceID];
                for (uint32_t indexId = 0; indexId < pFace->mNumIndices; ++indexId)
                {
                    //SR: This "can't" exceed USHRT_MAX since we use the import flag in assimp to max out before it.
                    uint32_t vertId = pFace->mIndices[indexId];
                    if (vertId > USHRT_MAX)
                    {
                        LOG_ERROR("Assimp failed to split large mesh!");
                    }
                    indices.push_back((uint16_t)vertId);
                }
            }
        }

        uValue = (int32_t)indices.size();
        WRITE_VALUE(uValue);

        if (uValue)
        {
            WRITE_VALUES(indices[0], (int32_t)indices.size());
        }

        uValue = PrimitiveType_TRIANGLE;
        switch (pLayoutMesh->mPrimitiveTypes)
        {
        case aiPrimitiveType_POINT:
            uValue = PrimitiveType_POINT;
            break;

        case aiPrimitiveType_TRIANGLE:
            uValue = PrimitiveType_TRIANGLE;
            break;

        case aiPrimitiveType_LINE:
            uValue = PrimitiveType_LINE;
            break;

        case aiPrimitiveType_POLYGON:
            uValue = PrimitiveType_POINT;
            LOG_ERROR("Unsupported assimp primitive type : %d [converted file may crash on load]", pLayoutMesh->mPrimitiveTypes);
            break;
        default:
            LOG_ERROR("Unsupported assimp primitive type : %d", pLayoutMesh->mPrimitiveTypes);
            continue;
        }

        WRITE_VALUE(uValue);

        int two_sided;
        uint32_t max = 1;
        if ((AI_SUCCESS == aiGetMaterialIntegerArray(pLayoutMaterial, AI_MATKEY_TWOSIDED, &two_sided, &max)) && two_sided)
        {
            uValue = 0;
        }
        else
        {
            uValue = 0;
        }
        WRITE_VALUE(uValue);

        aiColor3D AmbientColor, DiffuseColor, SpecularColor;
        float fShininess, fAlpha;
        if (AI_SUCCESS != pLayoutMaterial->Get(AI_MATKEY_COLOR_AMBIENT, AmbientColor))
        {
            AmbientColor = aiColor3D(0.25f, 0.25f, 0.25f);
        }
        if (AI_SUCCESS != pLayoutMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, DiffuseColor))
        {
            DiffuseColor = aiColor3D(0.5f, 0.5f, 0.5f);
        }
        if (AI_SUCCESS != pLayoutMaterial->Get(AI_MATKEY_COLOR_SPECULAR, SpecularColor))
        {
            SpecularColor = aiColor3D(1.0f, 1.0f, 1.0f);
        }
        if (AI_SUCCESS != pLayoutMaterial->Get(AI_MATKEY_SHININESS, fShininess))
        {
            fShininess = 16.0f;
        }
        if (AI_SUCCESS != pLayoutMaterial->Get(AI_MATKEY_OPACITY, fAlpha))
        {
            fAlpha = 1.0f;
        }
        WRITE_VALUE(AmbientColor);
        WRITE_VALUE(DiffuseColor);
        WRITE_VALUE(SpecularColor);
        WRITE_VALUE(fShininess);
        WRITE_VALUE(fAlpha);

        aiString texPath;
        aiColor4D diffuse;
        if (AI_SUCCESS == pLayoutMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texPath))
        {
            std::string sFullfilename = texPath.C_Str();
            std::string sFile = CFileExportSTUFormat::RemoveFoldersFromPaths(sFullfilename);
            std::string texPathStr(sFile);

            uValue = TextureType_DIFFUSE;
            WRITE_VALUE(uValue);
            if(strncmp(texPathStr.c_str(), AI_EMBEDDED_TEXNAME_PREFIX, strlen(AI_EMBEDDED_TEXNAME_PREFIX)) == 0)
            {
                size_t idx_texture = atoi(texPathStr.substr(strlen(AI_EMBEDDED_TEXNAME_PREFIX)).c_str());
                std::string embeddedTextureName = "InternalTexture(" + std::to_string(idx_texture) + ")";

                uSize += CFileExportSTUFormat::CopyString(embeddedTextureName.c_str(), &Data);
            }
            else
            {
                CImagePreProcess::ReplaceUnrecognizedInternalTextureFormats(sFile);
                uSize += CFileExportSTUFormat::CopyString(sFile.c_str(), &Data);
            }
        }
        else if (AI_SUCCESS == aiGetMaterialColor(pLayoutMaterial, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
        {
            float fR = diffuse.r;
            float fG = diffuse.g;
            float fB = diffuse.b;
            float fA = 0.05f;
            pLayoutMaterial->Get(AI_MATKEY_OPACITY, fA);    //This is sometimes *very* wrong...
            uValue = TextureType_COLOR_DIFFUSE;
            WRITE_VALUE(uValue);
            WRITE_VALUE(fA);
        }
        else
        {
            uValue = TextureType_UNKNOWN;
            WRITE_VALUE(uValue);
        }
        if (AI_SUCCESS == pLayoutMaterial->GetTexture(aiTextureType_HEIGHT, 0, &texPath))
        {
            std::string sFullfilename = texPath.C_Str();
            std::string sFile = CFileExportSTUFormat::RemoveFoldersFromPaths(sFullfilename);
            std::string texPathStr(sFile);

            uValue = TextureType_HEIGHT;
            WRITE_VALUE(uValue);

            if (strncmp(texPathStr.c_str(), AI_EMBEDDED_TEXNAME_PREFIX, strlen(AI_EMBEDDED_TEXNAME_PREFIX)) == 0)
            {
                size_t idx_texture = atoi(texPathStr.substr(strlen(AI_EMBEDDED_TEXNAME_PREFIX)).c_str());
                std::string embeddedTextureName = "InternalTexture(" + std::to_string(idx_texture) + ")";

                uSize += CFileExportSTUFormat::CopyString(embeddedTextureName.c_str(), &Data);
            }
            else
            {
                uSize += CFileExportSTUFormat::CopyString(texPathStr.c_str(), &Data);
            }
        }
        else
        {
            uValue = TextureType_UNKNOWN;
            WRITE_VALUE(uValue);
        }
    }
    m_TotalMeshCount += pLayoutNode->mNumMeshes;

    std::string sChunkname = std::string("Model:") + std::to_string(m_uSubModelCount++);

#if STU_EXPORT_SEQUENTIAL
    m_Export.AppendChunkToFile(m_sSTUPath, sChunkname, &Data.at(0), (int32_t)Data.size());
#else
    m_Export.WriteChunk(sChunkname, &Data.at(0), (int32_t)Data.size());
#endif

    for (uint32_t i = 0; i < pLayoutNode->mNumChildren; ++i)
    {
        ExportSubTree(pLayoutNode->mChildren[i], m_uSubModelCount);
    }
}
