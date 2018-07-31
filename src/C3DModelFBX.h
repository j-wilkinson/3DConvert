#ifndef _YES_3D_MODEL_FBX
#define _YES_3D_MODEL_FBX

#include "CFileExportSTUFormat.h"
#include "C3DModelDataStructures.h"

#include <fbxsdk.h>
#include <map>

class C3DModelFBX
{
public:
    C3DModelFBX();
    virtual ~C3DModelFBX();

    bool ExportToSTUFormat(const std::string &path);
    bool ExportToSTUFormat(const std::string &path, bool bFlipUV = true);

private:
    void ParseSkeletons();
    void ParseSubSkeletons(FbxNode* pNode);
    bool ParseAnimations();
    bool ParseAnimationLayer(Animation &animation, FbxTime &start, FbxTime &end, FbxAnimLayer* pAnimLayer, FbxNode* pNode);
    void ExportAnimations();
    void ExportTextures();
    void ExportSceneTree();
    void ExportSubTree(FbxNode* pNode);
    void ExportBones();
    void ExportVertices(CFileExportSTUFormat &Export, const std::string &path, std::string sChunkname, FbxMesh *pMesh, FbxTexture *pDiffuseTexture);
    void LoadBones(FbxMesh *pMesh);

    std::string m_path;
    uint32_t m_uSubModelCount;
    uint32_t m_uSubModelVertexCount;
    bool m_bFlipUVonY;

    std::map<uint32_t, uint32_t> m_BoneMapping; // maps a bone name to its index
    uint32_t m_uNumBones;
    std::vector<BoneInfo> m_BoneInfo;
    std::vector<VertexBoneData> m_Bones;
    std::vector<Animation> mAnimations;
    VertexDataType m_VertexDataType;
    bool m_bHasAnimations;

    FbxManager* m_pFBXManager;
    FbxScene* m_pFBXScene;
    FbxArray<FbxString*> m_AnimStackNameArray;
    FbxAnimLayer * m_CurrentAnimLayer;
    std::vector<FbxMesh*> m_fbxSkinMeshes;
    std::vector<FbxSkeleton*> m_fbxSkeletons;
    CFileExportSTUFormat m_Export;
    std::string m_sSTUPath;
};

#endif // _YES_3D_MODEL_FBX
