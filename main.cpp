#include <iostream>
#include <vector>
#include <stdexcept>
#include <random>
#include <cstdint>
#include <algorithm>

typedef std::uint64_t u64;
typedef std::int64_t  i64;
typedef std::uint32_t u32;
typedef std::int32_t  i32;
typedef std::uint16_t u16;
typedef std::int16_t  i16;
typedef unsigned char  u8;

struct RandomWalk {
    u64 value;
    u64 mask;

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

    template<typename T>
    bool _packN(const u64* input) {
        for (int i = 0; i < 16; i++) {
            T bits = static_cast<T>(input[i]);
            if (!stream_.put_raw(bits)) {
                return false;
            }
        }
        return true;
    }

    bool _pack1(const u64* input) {
        u16 bits = 0;
        for (int i = 0; i < 16; i++) {
            bits |= static_cast<u16>((input[i] & 1) << i);
        }
        if (!stream_.put_raw(bits)) {
            return false;
        }
        return true;
    }

    bool _pack2(const u64* input) {
        u32 bits = 0;
        for (int i = 0; i < 16; i++) {
            bits |= static_cast<u32>((input[i] & 3) << 2*i);
        }
        if (!stream_.put_raw(bits)) {
            return false;
        }
        return true;
    }

    bool _pack3(const u64* input) {
        u32 bits0 = 0;
        u16 bits1 = 0;
        bits0 |= static_cast<u32>((input[0]  & 7));
        bits0 |= static_cast<u32>((input[1]  & 7) << 3);
        bits0 |= static_cast<u32>((input[2]  & 7) << 6);
        bits0 |= static_cast<u32>((input[3]  & 7) << 9);
        bits0 |= static_cast<u32>((input[4]  & 7) << 12);
        bits0 |= static_cast<u32>((input[5]  & 7) << 15);
        bits0 |= static_cast<u32>((input[6]  & 7) << 18);
        bits0 |= static_cast<u32>((input[7]  & 7) << 21);
        bits0 |= static_cast<u32>((input[8]  & 7) << 24);
        bits0 |= static_cast<u32>((input[9]  & 7) << 27);
        bits0 |= static_cast<u32>((input[10] & 3) << 30);
        bits1 |= static_cast<u32>((input[10] & 4) >> 2);
        bits1 |= static_cast<u32>((input[11] & 7) << 1);
        bits1 |= static_cast<u32>((input[12] & 7) << 4);
        bits1 |= static_cast<u32>((input[13] & 7) << 7);
        bits1 |= static_cast<u32>((input[14] & 7) << 10);
        bits1 |= static_cast<u32>((input[15] & 7) << 13);
        if (!stream_.put_raw(bits0)) {
            return false;
        }
        if (!stream_.put_raw(bits1)) {
            return false;
        }
        return true;
    }

    bool _pack4(const u64* input) {
        u64 bits0 = 0;
        bits0 |= static_cast<u64>((input[0]  & 0xF));
        bits0 |= static_cast<u64>((input[1]  & 0xF) << 4);
        bits0 |= static_cast<u64>((input[2]  & 0xF) << 8);
        bits0 |= static_cast<u64>((input[3]  & 0xF) << 12);
        bits0 |= static_cast<u64>((input[4]  & 0xF) << 16);
        bits0 |= static_cast<u64>((input[5]  & 0xF) << 20);
        bits0 |= static_cast<u64>((input[6]  & 0xF) << 24);
        bits0 |= static_cast<u64>((input[7]  & 0xF) << 28);
        bits0 |= static_cast<u64>((input[8]  & 0xF) << 32);
        bits0 |= static_cast<u64>((input[9]  & 0xF) << 36);
        bits0 |= static_cast<u64>((input[10] & 0xF) << 40);
        bits0 |= static_cast<u64>((input[11] & 0xF) << 44);
        bits0 |= static_cast<u64>((input[12] & 0xF) << 48);
        bits0 |= static_cast<u64>((input[13] & 0xF) << 52);
        bits0 |= static_cast<u64>((input[14] & 0xF) << 52);
        bits0 |= static_cast<u64>((input[15] & 0xF) << 56);
        if (!stream_.put_raw(bits0)) {
            return false;
        }
        return true;
    }

    bool _pack5(const u64* input) {
        u64 bits0 = 0;
        u16 bits1 = 0;
        bits0 |= static_cast<u64>((input[0]  & 0x1F));
        bits0 |= static_cast<u64>((input[1]  & 0x1F) << 5);
        bits0 |= static_cast<u64>((input[2]  & 0x1F) << 10);
        bits0 |= static_cast<u64>((input[3]  & 0x1F) << 15);
        bits0 |= static_cast<u64>((input[4]  & 0x1F) << 20);
        bits0 |= static_cast<u64>((input[5]  & 0x1F) << 25);
        bits0 |= static_cast<u64>((input[6]  & 0x1F) << 30);
        bits0 |= static_cast<u64>((input[7]  & 0x1F) << 35);
        bits0 |= static_cast<u64>((input[8]  & 0x1F) << 40);
        bits0 |= static_cast<u64>((input[9]  & 0x1F) << 45);
        bits0 |= static_cast<u64>((input[10] & 0x1F) << 50);
        bits0 |= static_cast<u64>((input[11] & 0x1F) << 55);
        bits0 |= static_cast<u64>((input[12] & 0x0F) << 60);
        bits1 |= static_cast<u32>((input[12] & 0x10) >> 4);
        bits1 |= static_cast<u32>((input[13] & 0x1F) << 1);
        bits1 |= static_cast<u32>((input[14] & 0x1F) << 6);
        bits1 |= static_cast<u32>((input[15] & 0x1F) << 11);
        if (!stream_.put_raw(bits0)) {
            return false;
        }
        if (!stream_.put_raw(bits1)) {
            return false;
        }
        return true;
    }

    bool _pack6(const u64* input) {
        u64 bits0 = 0;
        u32 bits1 = 0;
        bits0 |= static_cast<u64>((input[0]  & 0x3F));
        bits0 |= static_cast<u64>((input[1]  & 0x3F) << 6);
        bits0 |= static_cast<u64>((input[2]  & 0x3F) << 12);
        bits0 |= static_cast<u64>((input[3]  & 0x3F) << 18);
        bits0 |= static_cast<u64>((input[4]  & 0x3F) << 24);
        bits0 |= static_cast<u64>((input[5]  & 0x3F) << 30);
        bits0 |= static_cast<u64>((input[6]  & 0x3F) << 36);
        bits0 |= static_cast<u64>((input[7]  & 0x3F) << 42);
        bits0 |= static_cast<u64>((input[8]  & 0x3F) << 48);
        bits0 |= static_cast<u64>((input[9]  & 0x3F) << 54);
        bits0 |= static_cast<u64>((input[10] & 0x0F) << 60);
        bits1 |= static_cast<u32>((input[10] & 0x30) >> 4);
        bits1 |= static_cast<u32>((input[11] & 0x3F) << 2);
        bits1 |= static_cast<u32>((input[12] & 0x3F) << 8);
        bits1 |= static_cast<u32>((input[13] & 0x3F) << 14);
        bits1 |= static_cast<u32>((input[14] & 0x3F) << 20);
        bits1 |= static_cast<u32>((input[15] & 0x3F) << 26);
        if (!stream_.put_raw(bits0)) {
            return false;
        }
        if (!stream_.put_raw(bits1)) {
            return false;
        }
        return true;
    }

    bool _pack7(const u64* input) {
        u64 bits0 = 0;
        u32 bits1 = 0;
        u16 bits2 = 0;
        bits0 |= static_cast<u64>((input[0]  & 0x7F));
        bits0 |= static_cast<u64>((input[1]  & 0x7F) << 7);
        bits0 |= static_cast<u64>((input[2]  & 0x7F) << 14);
        bits0 |= static_cast<u64>((input[3]  & 0x7F) << 21);
        bits0 |= static_cast<u64>((input[4]  & 0x7F) << 28);
        bits0 |= static_cast<u64>((input[5]  & 0x7F) << 35);
        bits0 |= static_cast<u64>((input[6]  & 0x7F) << 42);
        bits0 |= static_cast<u64>((input[7]  & 0x7F) << 49);
        bits0 |= static_cast<u64>((input[8]  & 0x7F) << 56);
        bits0 |= static_cast<u64>((input[9]  & 0x01) << 63);
        bits1 |= static_cast<u32>((input[9]  & 0x7E) >> 1);
        bits1 |= static_cast<u32>((input[10] & 0x7F) << 6);
        bits1 |= static_cast<u32>((input[11] & 0x7F) << 13);
        bits1 |= static_cast<u32>((input[12] & 0x7F) << 20);
        bits1 |= static_cast<u32>((input[13] & 0x1F) << 27);
        bits2 |= static_cast<u16>((input[13] & 0x60) >> 5);
        bits2 |= static_cast<u16>((input[14] & 0x7F) << 2);
        bits2 |= static_cast<u16>((input[15] & 0x7F) << 9);
        if (!stream_.put_raw(bits0)) {
            return false;
        }
        if (!stream_.put_raw(bits1)) {
            return false;
        }
        if (!stream_.put_raw(bits2)) {
            return false;
        }
        return true;
    }

    template<typename T>
    void _shiftN(u64* input) {
        for (int i = 0; i < 16; i++) {
            input[i] >>= sizeof(T);
        }
    }

    bool pack(const u64* input, int n) {
        switch(n) {
        case 0:
            return true;
        case 1:
            return _pack1(input);
        case 2:
            return _pack2(input);
        case 3:
            return _pack3(input);
        case 4:
            return _pack4(input);
        case 5:
            return _pack5(input);
        case 6:
            return _pack6(input);
        case 7:
            return _pack7(input);
        case 8:
            return _packN<u8>(input);
        case 9:
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack1(input);
        case 10:
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack2(input);
        case 11:
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack3(input);
        case 12:
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack4(input);
        case 13:
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack5(input);
        case 14:
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack6(input);
        case 15:
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack7(input);
        case 16:
            return _packN<u16>(input);
        case 17:
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _pack1(input);
        case 18:
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _pack2(input);
        case 19:
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _pack3(input);
        case 20:
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _pack4(input);
        case 21:
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _pack5(input);
        case 22:
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _pack6(input);
        case 23:
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _pack7(input);
        case 24:
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _packN<u8>(input);
        }
    }

    bool dumb_pack(const u64* input, int n) {
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
