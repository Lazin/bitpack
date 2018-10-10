#include <iostream>
#include <vector>
#include <stdexcept>
#include <random>

typedef uint64_t u64;
typedef int64_t i64;
typedef uint32_t u32;
typedef int32_t i32;
typedef unsigned char u8;

struct RandomWalk {
    std::random_device                  randdev;
    std::mt19937                        generator;
    std::normal_distribution<double>    distribution;
    double                              value;

    RandomWalk(double start, double mean, double stddev)
        : generator(randdev())
        , distribution(mean, stddev)
        , value(start)
    {
    }

    double generate() {
        value += distribution(generator);
        return value;
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

    Encoder(MemoryStream& stream) : stream_(stream) {}

    bool pack(const u64* input, int size, int n) {
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

    void unpack(u64* output, int size, int n) {
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

int main(int argc, char *argv[])
{
    const size_t N = 1000000;
    RandomWalk rwalk(0.0, 10.1, 0.01);
    MemoryStream stream(0x10000);
    Encoder encoder(stream);
    for (size_t i = 0; i < N; i++) {
        union {
            double x;
            u64 bits;
        } curr;
        curr.x = rwalk.generate();
    }
    return 0;
}
