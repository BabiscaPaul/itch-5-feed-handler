#pragma once

#include <cstddef> 
#include <cstdint>  
#include <cstdio>    
#include <fcntl.h>  
#include <sys/mman.h> 
#include <sys/stat.h> 
#include <unistd.h>  

class MmapReader {
private:
    const uint8_t* m_data{nullptr};
    size_t m_size{0};

public:
    MmapReader(const char* filepath) {
        int fd = open(filepath, O_RDONLY);
        if (fd < 0) {
            perror("open failed");
            return;
        }

        struct stat stat_buffer;
        fstat(fd, &stat_buffer);
        m_size = static_cast<size_t>(stat_buffer.st_size);

        m_data = static_cast<const uint8_t*>(
            mmap(nullptr, m_size, PROT_READ, MAP_PRIVATE, fd, 0)
        );

        if (m_data == MAP_FAILED) {
            perror("mmap");
            m_data = nullptr;
            m_size = 0;
        }

        close(fd);
    }

    ~MmapReader() {
        if (m_data) {
            munmap(const_cast<uint8_t*>(m_data), m_size);
        }
    }

    MmapReader(const MmapReader&) = delete;
    MmapReader& operator=(const MmapReader&) = delete;

    const uint8_t* data() const { return m_data; }
    size_t size() const { return m_size; }
    bool is_valid() const { return m_data != nullptr; }
};
