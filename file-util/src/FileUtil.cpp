#include <FileUtil.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#ifdef HAVE_LOGGER
#include <log.h>
#endif

bool FileUtil::exists(std::string const& path) {
    return access(path.c_str(), F_OK) == 0;
}

#ifdef __linux__
// Workaround for linux/aarch64 (ubuntu 20.04, 5.4.0-1021-raspi) running armhf version
// Failed to detect if some dirs are a directory
#define stat stat64
#endif
bool FileUtil::isDirectory(std::string const& path) {
    struct stat s;
    return !stat(path.c_str(), &s) && S_ISDIR(s.st_mode);
}
#ifdef __linux__
#undef stat
#endif


std::string FileUtil::getParent(std::string const& path) {
    auto iof = path.rfind('/');
    if (iof == std::string::npos)
        return std::string();
    bool endsWithSlash = iof == path.length() - 1;
    while (iof > 0 && path[iof - 1] == '/')
        iof--;
    auto ret = path.substr(0, iof);
    if(endsWithSlash) {
        return FileUtil::getParent(ret);
    }
    return ret;
}

void FileUtil::mkdirRecursive(std::string const& path) {
    if (isDirectory(path))
        return;
    if (exists(path))
        throw std::runtime_error(std::string("File exists and is not a directory: ") + path);
    mkdirRecursive(getParent(path));
    if (mkdir(path.c_str(), 0744) != 0)
        throw std::runtime_error(std::string("mkdir failed, path = ") + path);
}

bool FileUtil::readFile(std::string const &path, std::string &out) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
#if defined(HAVE_LOGGER) && !defined(NDEBUG)
        Log::error("FileUtil", "readFile: not found: %s\n", path.c_str());
#endif
        return false;
    }
    struct stat sr;
    if (fstat(fd, &sr) < 0 || (sr.st_mode & S_IFDIR)) {
        close(fd);
#if defined(HAVE_LOGGER) && !defined(NDEBUG)
        Log::error("FileUtil", "readFile: opening a directory: %s\n", path.c_str());
#endif
        return false;
    }
    auto size = lseek(fd, 0, SEEK_END);
    if (size == (off_t) -1) {
#ifdef HAVE_LOGGER
        Log::error("FileUtil", "readFile: lseek error\n");
#endif
        close(fd);
        return false;
    }
    out.resize((size_t) size);
    lseek(fd, 0, SEEK_SET);
    for (size_t o = 0; o < (size_t) size; ) {
        int res = read(fd, &out[o], size - o);
        if (res < 0) {
#ifdef HAVE_LOGGER
            Log::error("FileUtil", "readFile: read error\n");
#endif
            close(fd);
            return false;
        }
        o += res;
    }
    close(fd);
    return true;
}