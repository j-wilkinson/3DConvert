#ifndef EXPORT_STU_H_
#define EXPORT_STU_H_

#include <string>
#include <cstring>

#define YI_NULL nullptr
#include <cstdint>
#include <vector>

class CFileExportSTUFormat
{
public:

    CFileExportSTUFormat();
    virtual ~CFileExportSTUFormat();

    static const uint32_t uMaxTextLength = 255;

    enum STU_IMPORT_EXPORT_FLAGS
    {
        STU_IMPORT_EXPORT_FLAGS_NONE = 0,
        STU_IMPORT_EXPORT_PRINT_CHUNK_INFO = 1,
        STU_IMPORT_EXPORT_PARSE_ONLY = 2,
        STU_IMPORT_EXPORT_SHOW_PROFILE = 4,
        STU_IMPORT_EXPORT_FLAGS_LIMIT = 4,
        STU_IMPORT_EXPORT_FLAGS_ALL = 7,
    };
    struct STU_HEADER
    {
        unsigned char Leader[2];
        char Name[19];
        char Zero;
        unsigned char ChunkSizeInfo[4];
        uint32_t NameHash;
        STU_HEADER()
        {
            memset(this, 0, sizeof(STU_HEADER));
            Leader[0] = '$';
            Leader[1] = '$';
        }
    };
    struct STU_FILE_HEADER
    {
        unsigned char Magic[3];
        unsigned char Version[3];
        unsigned char FileSizeInfo[4];
        STU_FILE_HEADER()
        {
            Magic[0] = 'S';
            Magic[1] = 'T';
            Magic[2] = 'U';
            Version[0] = '0';
            Version[1] = '.';
            Version[2] = '1';
        }
    };

    class STU_CHUNK
    {
        friend class CFileExportSTUFormat;
    public:
        STU_HEADER Header;
        STU_CHUNK()
        {
            m_Data.clear();
        }
        ~STU_CHUNK()
        {
            std::vector< uint8_t >().swap(m_Data);
            m_uAllocation = 0;
        }
        bool AllocateForData(uint32_t nSize)
        {
            m_Data.resize(nSize);
            if (m_Data.size() == nSize)
            {
                m_uAllocation = nSize;
                return true;
            }
            else
            {
                m_uAllocation = 0;
                return false;
            }
        }
        void SetData(uint8_t * pData, uint32_t uLen)
        {
            m_Data.resize(uLen);
            m_uAllocation = uLen;
            m_Data.assign(pData, pData + uLen);
            }
        std::vector<uint8_t> GetDataVector() const
            {
            return m_Data;
        }
        uint8_t * GetData() const
        {
            return (uint8_t *)&m_Data[0];
        }
        uint32_t GetDataSize() const
        {
            return m_uAllocation;
        }

    protected:
        std::vector<uint8_t> m_Data;
        uint32_t m_uAllocation;
    };

    static unsigned char m_ucMagic[];
    static unsigned char m_ucVersion[];

    /* Save stored file data  */
    bool ExportFile(const std::string &path);

    /* Check is the files exists */
    bool CheckFileExists(const std::string &path);

    /* store new chunk data */
    bool WriteChunk(const std::string &sName, void * pData, uint32_t uLength);

    /* append chunk data to an existing file. This cna be used for very large models to break apart the process for memory optimization, or to add new features to save files. */
    bool AppendChunkToFile(const std::string &path, const std::string &sName, void * pData, uint32_t uLength);

    /* Copy a string into our output array, respecting the maximum size */
    static uint32_t CopyString(const char * pString, std::vector< uint8_t > * Target);

    /* Remove folders from image paths. This is required because some models have full local computer paths which are not portable  */
    static std::string RemoveFoldersFromPaths(const std::string &sPath);

    /* String compare function for extensions */
    static bool EndsWithIgnoreCase(std::string fullString, std::string ending);

    /* Make a hash code from the name string for faster searches */
    static uint32_t MakeHashFromName(const std::string &sName);

private:

    std::vector< STU_CHUNK *> m_Buffer;
    unsigned char * m_sVersion;
};

#endif
