/*
PARTIO SOFTWARE
Copyright 2013 Disney Enterprises, Inc. All rights reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

* The names "Disney", "Walt Disney Pictures", "Walt Disney Animation
Studios" or the names of its contributors may NOT be used to
endorse or promote products derived from this software without
specific prior written permission from Walt Disney Pictures.

Disclaimer: THIS SOFTWARE IS PROVIDED BY WALT DISNEY PICTURES AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
IN NO EVENT SHALL WALT DISNEY PICTURES, THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/

#include <iostream>
#include <sstream>
#include "../core/Mutex.h"
#include "../Partio.h"
#include "readers.h"

ENTER_PARTIO_NAMESPACE

// reader and writer code
typedef ParticlesDataMutable* (*READER_FUNCTION)(const char*,const bool);
typedef bool (*WRITER_FUNCTION)(const char*,const ParticlesData&,const bool);

PartioMutex initializationMutex;

std::map<std::string, READER_FUNCTION>&
readers()
{
    static std::map<std::string, READER_FUNCTION> data;
    static bool initialized = false;
    if(!initialized){
    initializationMutex.lock();
        data["bgeo"]        = readBGEO;
        data["bhclassic"]   = readBGEO;
        data["geo"]         = readGEO;
        data["hclassic"]    = readGEO;
        data["pdb"]         = readPDB;
        data["pdb32"]       = readPDB32;
        data["pdb64"]       = readPDB64;
        data["pda"]         = readPDA;
        data["mc"]          = readMC;
        data["ptc"]         = readPTC;
        data["pdc"]         = readPDC;
        data["prt"]         = readPRT;
        data["bin"]         = readBIN;
        data["pts"]         = readPTS;
        data["xyz"]         = readXYZ;
        data["pcd"]         = readPCD;
        data["ptf"]         = readPTC;
        data["itbl"]        = readBGEO;
        data["atbl"]        = readBGEO;
        data["rpc"]         = readRPC;
        initialized = true;
        initializationMutex.unlock();
    }
    return data;
}

std::map<std::string, WRITER_FUNCTION>&
writers()
{
    static std::map<std::string, WRITER_FUNCTION> data;
    static bool initialized=false;
    if(!initialized){
    initializationMutex.lock();
        data["bgeo"]        = writeBGEO;
        data["bhclassic"]   = writeBGEO;
        data["geo"]         = writeGEO;
        data["pdb"]         = writePDB;
        data["pdb32"]       = writePDB32;
        data["pdb64"]       = writePDB64;
        data["pda"]         = writePDA;
        data["ptc"]         = writePTC;
        data["rib"]         = writeRIB;
        data["pdc"]         = writePDC;
        data["prt"]         = writePRT;
        data["bin"]         = writeBIN;
        data["pcd"]         = writePCD;
        data["ptf"]         = writePTC;
        data["itbl"]        = writeBGEO;
        data["atbl"]        = writeBGEO;
        //data["rpc"]       = writeRPC;
        initialized = true;
        initializationMutex.unlock();
    }
    return data;
}

//! Gives extension of a file ignoring any trailing .gz
//! i.e. for 'foo.pdb.gz' it gives 'pdb', for 'foo.pdb' it gives 'pdb'
bool extensionIgnoringGz(const std::string& filename, std::string& ret, bool& endsWithGz)
{
    size_t period=filename.rfind('.');
    endsWithGz=false;
    if (period == std::string::npos) {
        std::cerr << "Partio: No extension detected in filename" << std::endl;
        return false;
    }
    std::string extension = filename.substr(period+1);
    if (extension == "gz") {
        endsWithGz=true;
        size_t period2=filename.rfind('.',period-1);
        if (period2 == std::string::npos) {
            std::cerr << "Partio: No extension detected in filename" << std::endl;
            return false;
        }
        std::string extension2 = filename.substr(period2 + 1, period - period2 - 1);
        ret = extension2;
    } else {
        ret = extension;
    }
    return true;
}

ParticlesDataMutable*
read(const char* c_filename)
{
    std::string filename(c_filename);
    std::string extension;
    bool endsWithGz;
    if (!extensionIgnoringGz(filename,extension,endsWithGz)) return 0;
    std::map<std::string, READER_FUNCTION>::iterator i = readers().find(extension);
    if (i == readers().end()) {
        std::cerr << "Partio READ: No reader defined for extension " << extension << std::endl;
        return 0;
    }
    return (*i->second)(c_filename,false);
}

ParticlesInfo*
readHeaders(const char* c_filename)
{
    std::string filename(c_filename);
    std::string extension;
    bool endsWithGz;
    if (!extensionIgnoringGz(filename,extension,endsWithGz)) return 0;
    std::map<std::string, READER_FUNCTION>::iterator i = readers().find(extension);
    if (i == readers().end()){
        std::cerr << "Partio READ HEADERS: No reader defined for extension " << extension << std::endl;
        return 0;
    }
    return (*i->second)(c_filename,true);
}

void
write(const char* c_filename, const ParticlesData& particles, const bool forceCompressed)
{
    std::string filename(c_filename);
    std::string extension;
    bool endsWithGz;
    if(!extensionIgnoringGz(filename,extension,endsWithGz)) return;
    std::map<std::string, WRITER_FUNCTION>::iterator i = writers().find(extension);
    if (i == writers().end()){
        std::cerr << "Partio: No writer defined for extension " << extension << std::endl;
        return;
    }
    (*i->second)(c_filename,particles,forceCompressed || endsWithGz);
}

std::vector<std::string>
supportedReadFormats() {
    std::map<std::string, READER_FUNCTION>& _readers = readers();
    std::vector<std::string> ret;
    ret.reserve(_readers.size());
    for (std::map<std::string, READER_FUNCTION>::iterator it = _readers.begin(); it != _readers.end(); ++it) {
        ret.push_back(it->first);
    }

    return ret;
}

std::vector<std::string>
supportedWriteFormats() {
    std::map<std::string, WRITER_FUNCTION>& _writers = writers();
    std::vector<std::string> ret;
    ret.reserve(_writers.size());
    for (std::map<std::string, WRITER_FUNCTION>::iterator it = _writers.begin(); it != _writers.end(); ++it) {
        ret.push_back(it->first);
    }

    return ret;
}

EXIT_PARTIO_NAMESPACE
