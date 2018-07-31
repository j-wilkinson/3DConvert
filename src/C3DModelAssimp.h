#ifndef _YES_3D_MODEL_ASSIMP
#define _YES_3D_MODEL_ASSIMP

#include "CFileExportSTUFormat.h"
#include "C3DModelDataStructures.h"

#include "assimp/Importer.hpp"
#include <map>

struct aiNode;
struct aiMesh;

class C3DModelAssimp
{
public:
    C3DModelAssimp();
    virtual ~C3DModelAssimp();

    bool ExportToSTUFormat(const std::string &path);
    bool ExportToSTUFormat(const std::string &path, bool bFlipUV = true);

private:

    bool ImportAssimp(const std::string &path, bool bFlipUV = true);
    void ParseAnimations();
    void ExportSubTree(const aiNode* pLayoutNode, uint32_t uIndex);
    void ExportSceneTree();
    void ExportBones();
    void ExportAnimations();
    void ExportTextures();
    void WriteVertexChunk(std::string sName, uint32_t uCurrentMesh, const aiMesh *pLayoutMesh, VertexDataType m_VertexDataType);

    struct MeshEntry {
        MeshEntry()
        {
            uBaseVertex = 0;
        }
        uint32_t uBaseVertex;
    };
    uint32_t m_TotalMeshCount;
    uint32_t m_uSubModelCount;
    uint32_t m_uSubModelVertexCount;

    std::vector<MeshEntry> m_Entries;

    Assimp::Importer m_importer;
    const aiScene * m_pAIScene;

    CFileExportSTUFormat m_Export;
    std::string m_sSTUPath;
    bool m_bFlipUVonY;

    std::map<uint32_t, uint32_t> m_BoneMapping; // maps a bone name to its index
    uint32_t m_uNumBones;
    std::vector<BoneInfo> m_BoneInfo;
    std::vector<VertexBoneData> m_Bones;
    std::vector<Animation> mAnimations;
    VertexDataType m_VertexDataType;
    bool m_bHasAnimations;
};

#endif // _YES_3D_MODEL_ASSIMP
