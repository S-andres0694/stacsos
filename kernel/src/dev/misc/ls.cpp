#include <stacsos/kernel/dev/misc/ls.h>
#include <stacsos/kernel/fs/file.h>
#include <stacsos/syscalls.h>
#include <stacsos/kernel/debug.h>

using namespace stacsos::kernel::dev;
using namespace stacsos::kernel::fs;
using namespace stacsos::kernel::dev::misc;
using namespace stacsos;

device_class ls::ls_device_class(device_class::root, "ls");

class ls_file : public file {
public:
    ls_file(ls &ls_device)
        : file((sizeof(final_product)))
        , ls_(ls_device)
    {
    }

    virtual size_t pread(void *buffer, size_t offset, size_t length) override
    {
        if (length < sizeof(final_product)) {
            return 0;
        }

        stacsos::final_product *result = (stacsos::final_product *)buffer;

        result->result.code = this->ls_.prod.result.code;
        result->result.result_code = this->ls_.prod.result.result_code;
        result->result.number_entries = this->ls_.prod.result.number_entries;
        memops::memcpy(result->entries, this->ls_.prod.entries, sizeof(directory_entry) * this->ls_.prod.result.number_entries);

        return sizeof(final_product);
    }

    virtual size_t pwrite(const void *buffer, size_t offset, size_t length) override
    {
        return 0;
    }
private :
    ls &ls_;
};

shared_ptr<file> ls::open_as_file() {
    return shared_ptr(new ls_file(*this));
}