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
        : file((sizeof(ls_result)))
        , ls_(ls_device)
    {
    }

    virtual size_t pread(void *buffer, size_t offset, size_t length) override
    {
        if (length < sizeof(ls_result)) {
            return 0;
        }

        stacsos::ls_result *result = (stacsos::ls_result *)buffer;

        result->code = this->ls_.result.code;
        result->result_code = this->ls_.result.result_code;
        result->number_entries = this->ls_.result.number_entries;

        return sizeof(ls_result);
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