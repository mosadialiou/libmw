#include <mw/file/File.h>
#include <mw/common/Logger.h>

#if defined(_WIN32)
#include <Windows.h>
#else
#include <unistd.h>
#endif

void File::Create()
{
    m_path.GetParent().CreateDirIfMissing();

    std::ifstream inFile(m_path.m_path, std::ios::in | std::ifstream::ate | std::ifstream::binary);
    if (inFile.is_open())
    {
        inFile.close();
    }
    else
    {
        LOG_INFO_F("File {} does not exist. Creating it now.", m_path);
        std::ofstream outFile(m_path.m_path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!outFile.is_open())
        {
            ThrowFile_F("Failed to create file: {}", m_path);
        }
        outFile.close();
    }
}

bool File::Exists() const
{
    return m_path.Exists() && !m_path.IsDirectory();
}

void File::Truncate(const uint64_t size)
{
    bool success = false;

#if defined(WIN32)
    HANDLE hFile = CreateFile(
        m_path.ToString().c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    LARGE_INTEGER li;
    li.QuadPart = size;
    success = SetFilePointerEx(hFile, li, NULL, FILE_BEGIN) && SetEndOfFile(hFile);

    CloseHandle(hFile);
#else
    success = (truncate(m_path.ToString().c_str(), size) == 0);
#endif

    if (!success)
    {
        ThrowFile_F("Failed to truncate {}", m_path);
    }
}

void File::Rename(const std::string& filename)
{
    if (m_path.IsDirectory())
    {
        // FUTURE: Support renaming directories?
        ThrowFile("Rename not implemented for directories");
    }

    const FilePath parent(m_path.GetParent());
    if (parent == m_path)
    {
        ThrowFile_F("Can't find parent path for {}", *this);
    }

    const FilePath destination = parent.GetChild(filename);
    if (destination.Exists())
    {
        destination.Remove();
    }

    std::error_code ec;
    filesystem::rename(m_path.m_path, destination.m_path, ec);
    if (ec)
    {
        ThrowFile_F("Failed to rename {} to {}", *this, destination);
    }

    m_path = destination.m_path;
}

std::vector<uint8_t> File::ReadBytes() const
{
    std::error_code ec;
    if (!filesystem::exists(m_path.m_path, ec) || ec)
    {
        ThrowFile_F("{} not found", *this);
    }

    std::ifstream file(m_path.m_path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        ThrowFile_F("Failed to open {} for reading", *this);
    }

    const size_t size = (size_t)filesystem::file_size(m_path.m_path, ec);

    std::vector<uint8_t> bytes((size_t)size);
    file.seekg(0, std::ios::beg);
    file.read((char*)bytes.data(), size);
    file.close();

    return bytes;
}

std::vector<uint8_t> File::ReadBytes(const size_t startIndex, const size_t numBytes) const
{
    std::error_code ec;
    if (!filesystem::exists(m_path.m_path, ec) || ec) {
        ThrowFile_F("{} not found", *this);
    }

    std::ifstream file(m_path.m_path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        ThrowFile_F("Failed to open {} for reading", *this);
    }

    const size_t size = (size_t)filesystem::file_size(m_path.m_path, ec);
    if (size < (startIndex + numBytes)) {
        ThrowFile_F("Failed to read {} bytes from {}.", numBytes, *this);
    }

    std::vector<uint8_t> bytes((size_t)numBytes);
    file.seekg(startIndex, std::ios::beg);
    file.read((char*)bytes.data(), numBytes);
    file.close();

    return bytes;
}

void File::Write(const std::vector<uint8_t>& bytes)
{
    std::ofstream file(m_path.m_path, std::ios::out | std::ios::binary | std::ios::app);
    if (!file.is_open())
    {
        ThrowFile_F("Failed to write to file: {}", m_path);
    }

    file.write((const char*)bytes.data(), bytes.size());
    file.close();
}

void File::Write(const size_t startIndex, const std::vector<uint8_t>& bytes, const bool truncate)
{
    if (!bytes.empty())
    {
        std::fstream file(m_path.m_path, std::ios::out | std::ios::binary | std::ios::app);
        if (!file.is_open())
        {
            ThrowFile_F("Failed to write to file: {}", m_path);
        }

        file.seekp(startIndex, std::ios::beg);
        file.write((const char*)bytes.data(), bytes.size());
        file.close();
    }

    if (truncate)
    {
        Truncate(startIndex + bytes.size());
    }
}

void File::WriteBytes(const std::unordered_map<uint64_t, uint8_t>& bytes)
{
    std::fstream file(m_path.m_path, std::ios_base::binary | std::ios_base::out | std::ios_base::in);

    for (auto iter : bytes)
    {
        file.seekp(iter.first);
        file.write((const char*)&iter.second, 1);
    }

    file.close();
}

size_t File::GetSize() const
{
    if (!m_path.Exists()) {
        return 0;
    }

    std::error_code ec;
    const size_t size = (size_t)filesystem::file_size(m_path.m_path, ec);
    if (ec)
    {
        ThrowFile_F("Failed to determine size of {}", *this);
    }

    return size;
}