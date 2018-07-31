#ifndef _YES_3D_MODEL_OBJ
#define _YES_3D_MODEL_OBJ

#include "CFileExportSTUFormat.h"

class C3DModelOBJ
{
public:
    C3DModelOBJ();
    virtual ~C3DModelOBJ();

    bool ExportToSTUFormat(const std::string &path);
    bool ExportToSTUFormat(const std::string &path, bool bFlipUV = true);
    void SetDefaultSolidColor(float fRed, float fGreen, float fBlue) { m_SolidColor[0] = fRed; m_SolidColor[1] = fGreen;  m_SolidColor[2] = fBlue; }

private:

    void ExportSceneTree();

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

    CFileExportSTUFormat m_Export;
    bool m_bFlipUVonY;
    uint32_t m_uUniqueOBJUnknownID;
    float m_SolidColor[3];
};

#endif // _YES_3D_MODEL_OBJ
