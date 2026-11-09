#include "FootprintLibrary.h"

#include <wx/dir.h>
#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <algorithm>

namespace
{
wxString LibraryError(const char* text)
{
    return wxString::FromUTF8(text);
}

} // namespace

FootprintLibrary::FootprintLibrary(const wxString& directory)
    : directory_(directory)
{
}

wxString FootprintLibrary::DefaultDirectory()
{
    wxFileName executable(wxStandardPaths::Get().GetExecutablePath());
    return executable.GetPath() + wxFILE_SEP_PATH + "footprints";
}

bool FootprintLibrary::EnsureSamples(wxString* error)
{
    wxFileName::Mkdir(directory_, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

    wxDir directory(directory_);
    wxString firstFile;
    if (directory.GetFirst(&firstFile, "*.fp", wxDIR_FILES))
    {
        return true;
    }

    struct Sample
    {
        const char* filename;
        Footprint footprint;
    };

    const std::vector<Sample> samples = {
        {"R_0805.fp", Footprint::CreateResistor0805()},
        {"HEADER2_1.fp", Footprint::CreateHeader2()},
        {"DIP14.fp", Footprint::CreateDip14()},
        {"SO14.fp", Footprint::CreateSo14()},
    };

    for (const Sample& sample : samples)
    {
        if (!Write(directory_ + wxFILE_SEP_PATH + sample.filename, sample.footprint, error))
        {
            return false;
        }
    }
    return true;
}

bool FootprintLibrary::Reload(wxString* error)
{
    entries_.clear();
    wxFileName::Mkdir(directory_, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

    wxDir directory(directory_);
    wxString filename;
    bool more = directory.GetFirst(&filename, "*.fp", wxDIR_FILES);
    while (more)
    {
        const wxString path = directory_ + wxFILE_SEP_PATH + filename;
        Footprint footprint;
        if (Read(path, footprint, error))
        {
            entries_.push_back({path, footprint.name, footprint.category});
        }
        more = directory.GetNext(&filename);
    }
    std::sort(entries_.begin(), entries_.end(),
              [](const FootprintLibraryEntry& lhs, const FootprintLibraryEntry& rhs)
              {
                  if (lhs.category != rhs.category)
                  {
                      return lhs.category < rhs.category;
                  }
                  return lhs.name < rhs.name;
              });
    return true;
}

bool FootprintLibrary::Read(const wxString& path, Footprint& footprint, wxString* error) const
{
    wxFile file(path, wxFile::read);
    if (!file.IsOpened())
    {
        if (error != nullptr)
        {
            *error = LibraryError("无法打开封装文件。");
        }
        return false;
    }

    const wxFileOffset length = file.Length();
    if (length < 0)
    {
        if (error != nullptr)
        {
            *error = LibraryError("封装文件长度无效。");
        }
        return false;
    }

    std::string bytes(static_cast<std::size_t>(length), '\0');
    if (length > 0 && file.Read(bytes.data(), static_cast<std::size_t>(length)) != length)
    {
        if (error != nullptr)
        {
            *error = LibraryError("读取封装文件失败。");
        }
        return false;
    }

    std::string parseError;
    if (!footprint.Deserialize(bytes, &parseError))
    {
        if (error != nullptr)
        {
            *error = wxString::FromUTF8(parseError);
        }
        return false;
    }
    return true;
}

bool FootprintLibrary::Write(const wxString& path, const Footprint& footprint, wxString* error) const
{
    wxFile file(path, wxFile::write);
    if (!file.IsOpened())
    {
        if (error != nullptr)
        {
            *error = LibraryError("无法写入封装文件。");
        }
        return false;
    }

    const std::string bytes = footprint.Serialize();
    if (file.Write(bytes.data(), bytes.size()) != bytes.size())
    {
        if (error != nullptr)
        {
            *error = LibraryError("写入封装文件失败。");
        }
        return false;
    }
    return true;
}

bool FootprintLibrary::Delete(const wxString& path, wxString* error) const
{
    if (!wxRemoveFile(path))
    {
        if (error != nullptr)
        {
            *error = LibraryError("删除封装文件失败。");
        }
        return false;
    }
    return true;
}
