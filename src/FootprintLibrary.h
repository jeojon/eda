#pragma once

#include "FootprintModel.h"

#include <wx/string.h>

#include <vector>

struct FootprintLibraryEntry
{
    wxString path;
    std::string name;
    std::string category;
};

class FootprintLibrary
{
public:
    explicit FootprintLibrary(const wxString& directory);

    static wxString DefaultDirectory();

    const wxString& Directory() const { return directory_; }
    const std::vector<FootprintLibraryEntry>& Entries() const { return entries_; }

    bool EnsureSamples(wxString* error = nullptr);
    bool Reload(wxString* error = nullptr);

    bool Read(const wxString& path, Footprint& footprint, wxString* error = nullptr) const;
    bool Write(const wxString& path, const Footprint& footprint, wxString* error = nullptr) const;
    bool Delete(const wxString& path, wxString* error = nullptr) const;

private:
    wxString directory_;
    std::vector<FootprintLibraryEntry> entries_;
};

