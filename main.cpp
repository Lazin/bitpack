#include <iostream>
#include <vector>
#include <stdexcept>
#include <random>
#include <cstdint>
#include <algorithm>

typedef std::uint64_t u64;
typedef std::int64_t i64;
typedef std::uint32_t u32;
typedef std::int32_t i32;
typedef unsigned char u8;

struct RandomWalk {
    u64                                 value;
    u64                                 mask;

    RandomWalk(u64 mask)
        : mask(mask)
    {
    }

    u64 generate() {
        value = static_cast<u64>(rand()) | (static_cast<u64>(rand()) << 32);
        return value & mask;
    }
};

class MemoryStream {
    std::vector<u8> data_;
    u8* pos_;
    u8* end_;
public:

    MemoryStream(size_t size)
        : data_(size)
        , pos_(data_.data())
        , end_(data_.data() + size)
    {
    }

    template <class TVal> bool put_raw(TVal value) {
        if ((end_ - pos_) < static_cast<i32>(sizeof(TVal))) {
            return false;
        }
        *reinterpret_cast<TVal*>(pos_) = value;
        pos_ += sizeof(value);
        return true;
    }

    template <class TVal> TVal read_raw() {
        size_t sz = sizeof(TVal);
        if ((end_ - pos_) < static_cast<i32>(sz)) {
            throw std::out_of_range("End-Of-Stream");
        }
        auto out = *reinterpret_cast<const TVal*>(pos_);
        pos_ += sz;
        return out;
    }

    void reset() {
        pos_ = data_.data();
    }
};

class Encoder {
    MemoryStream &stream_;
public:
    Encoder(MemoryStream& stream) : stream_(stream) {}

    bool _pack(const u64* input, int n) {

    }

    bool pack(const u64* input, int n) {
        int size = 16;
        u8 bits = 0;
        int ixbits = 0;
        for (int i = 0; i < size; i++) {
            u64 word = input[i];
            for (int ixword = 0; ixword < n; ixword++) {
                if (word & 1) {
                    bits |= (1 << ixbits);
                }
                ixbits++;
                word >>= 1;
                if (ixbits == 8) {
                    if (!stream_.put_raw(static_cast<u8>(bits))) {
                        return false;
                    }
                    ixbits = 0;
                    bits = 0;
                }
            }
        }
        if (ixbits != 0 && n != 0) {
            if (!stream_.put_raw(static_cast<u8>(bits))) {
                return false;
            }
        }
        return true;
    }

    void unpack(u64* output, int n) {
        int size = 16;
        u8 bits = 0;
        int bitindex = 8;
        for (auto ixout = 0; ixout < size; ixout++) {
            u64 current = 0;
            for (int ixbit = 0; ixbit < n; ixbit++) {
                if (bitindex == 8) {
                    bitindex = 0;
                    bits = stream_.template read_raw<u8>();
                }
                if (bits & 1) {
                    current |= 1ull << ixbit;
                }
                bits >>= 1;
                bitindex++;
            }
            output[ixout] = current;
        }
    }
};

int get_bit_width(u64 x) {
    if (x == 0) {
        return 64;
    }
    return 64 - __builtin_clzl(x);
}

int main(int argc, char *argv[])
{
    const size_t N = 1000000;
    const u64 mask = 0x7FFFFFul;
    RandomWalk rwalk(mask);
    MemoryStream stream(0x10000);
    Encoder encoder(stream);
    std::vector<u64> expected;
    const int stride = 16;
    for (size_t i = 0; i < N; i += stride) {
        u64 input[stride];
        for (int j = 0; j < stride; j++) {
            input[j] = rwalk.generate();
        }
        bool res = encoder.pack(input, get_bit_width(mask));
        if (!res) {
            break;
        }
        std::copy(input, input + stride, std::back_inserter(expected));
    }
    // Read back
    stream.reset();
    for (u32 i = 0; i < expected.size(); i += stride) {
        u64 output[stride];
        encoder.unpack(output, get_bit_width(mask));
        for (u32 j = 0; j < stride; j++) {
            if (output[j] != expected[i + j]) {
                throw "not equal";
            }
        }
    }
    return 0;
}
