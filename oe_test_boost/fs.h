// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_FSLOCAL_H
#define BITCOIN_FSLOCAL_H

#include <stdio.h>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

/** Filesystem operations and types */
namespace fs = boost::filesystem;

/** Bridge operations to C stdio */
namespace fsbridge {
    FILE *fopen(const fs::path& p, const char *mode);

    class FileLock
    {
    public:
        FileLock() = delete;
        FileLock(const FileLock&) = delete;
        FileLock(FileLock&&) = delete;
        explicit FileLock(const fs::path& file);
        ~FileLock();
        bool TryLock();
        std::string GetReason() { return reason; }

    private:
        std::string reason;
        void* hFile = (void*)-1; // INVALID_HANDLE_VALUE
    };

    std::string get_filesystem_error_message(const fs::filesystem_error& e);

    // GNU libstdc++ specific workaround for opening UTF-8 paths on Windows.
    //
    // On Windows, it is only possible to reliably access multibyte file paths through
    // `wchar_t` APIs, not `char` APIs. But because the C++ standard doesn't
    // require ifstream/ofstream `wchar_t` constructors, and the GNU library doesn't
    // provide them (in contrast to the Microsoft C++ library, see
    // https://stackoverflow.com/questions/821873/how-to-open-an-stdfstream-ofstream-or-ifstream-with-a-unicode-filename/822032#822032),
    // Boost is forced to fall back to `char` constructors which may not work properly.
    //
    // Work around this issue by creating stream objects with `_wfopen` in
    // combination with `__gnu_cxx::stdio_filebuf`. This workaround can be removed
    // with an upgrade to C++17, where streams can be constructed directly from
    // `std::filesystem::path` objects.

    typedef fs::ifstream ifstream;
    typedef fs::ofstream ofstream;
};

#endif // BITCOIN_FSLOCAL_H
