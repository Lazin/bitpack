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

    template <typename T>
    void _unpackN(u64* input, int shift) {
        for (int i = 0; i < 16; i++) {
            T val = static_cast<T>(stream_.read_raw<T>());
            input[i] |= static_cast<u64>(val) << shift;
        }
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

    void _unpack1(u64* output, int shift) {
        u16 bits = stream_.read_raw<u16>();
        for (int i = 0; i < 16; i++) {
            output[i] |= static_cast<u64>((bits & (3 << i)) >> i) << shift;
        }
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

    void _unpack2(u64* output, int shift) {
        u32 bits = stream_.read_raw<u32>();
        for (u32 i = 0; i < 16; i++) {
            output[i] |= static_cast<u64>((bits & (3u << 2*i)) >> 2*i) << shift;
        }
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

    void _unpack3(u64* output, int shift) {
        u32 bits0  = stream_.read_raw<u32>();
        u16 bits1  = stream_.read_raw<u16>();
        output[0]  |= ((bits0 & 7)) << shift;
        output[1]  |= ((bits0 & (7u <<  3)) >> 3)  << shift;
        output[2]  |= ((bits0 & (7u <<  6)) >> 6)  << shift;
        output[3]  |= ((bits0 & (7u <<  9)) >> 9)  << shift;
        output[4]  |= ((bits0 & (7u << 12)) >> 12) << shift;
        output[5]  |= ((bits0 & (7u << 15)) >> 15) << shift;
        output[6]  |= ((bits0 & (7u << 18)) >> 18) << shift;
        output[7]  |= ((bits0 & (7u << 21)) >> 21) << shift;
        output[8]  |= ((bits0 & (7u << 24)) >> 24) << shift;
        output[9]  |= ((bits0 & (7u << 27)) >> 27) << shift;
        output[10] |= (((bits0 & (3u << 30)) >> 30) | (bits1 & 4)) << shift;
        output[11] |= ((bits1 & (7u <<  1)) >> 1)  << shift;
        output[12] |= ((bits1 & (7u <<  4)) >> 4)  << shift;
        output[13] |= ((bits1 & (7u <<  7)) >> 7)  << shift;
        output[14] |= ((bits1 & (7u << 10)) >> 10) << shift;
        output[15] |= ((bits1 & (7u << 13)) >> 13) << shift;
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
        bits0 |= static_cast<u64>((input[14] & 0xF) << 56);
        bits0 |= static_cast<u64>((input[15] & 0xF) << 60);
        if (!stream_.put_raw(bits0)) {
            return false;
        }
        return true;
    }

    void _unpack4(u64* output, int shift) {
        u64 bits0  = stream_.read_raw<u64>();
        output[0]  |= ((bits0 & 0xF)) << shift;
        output[1]  |= ((bits0 & (15ull <<  4)) >>  4)  << shift;
        output[2]  |= ((bits0 & (15ull <<  8)) >>  8)  << shift;
        output[3]  |= ((bits0 & (15ull << 12)) >> 12)  << shift;
        output[4]  |= ((bits0 & (15ull << 16)) >> 16)  << shift;
        output[5]  |= ((bits0 & (15ull << 20)) >> 20)  << shift;
        output[6]  |= ((bits0 & (15ull << 24)) >> 24)  << shift;
        output[7]  |= ((bits0 & (15ull << 28)) >> 28)  << shift;
        output[8]  |= ((bits0 & (15ull << 32)) >> 32)  << shift;
        output[9]  |= ((bits0 & (15ull << 36)) >> 36)  << shift;
        output[10] |= ((bits0 & (15ull << 40)) >> 40)  << shift;
        output[11] |= ((bits0 & (15ull << 44)) >> 44)  << shift;
        output[12] |= ((bits0 & (15ull << 48)) >> 48)  << shift;
        output[13] |= ((bits0 & (15ull << 52)) >> 52)  << shift;
        output[14] |= ((bits0 & (15ull << 56)) >> 56)  << shift;
        output[15] |= ((bits0 & (15ull << 60)) >> 60)  << shift;
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

    void _unpack5(u64* output, int shift) {
        u64 bits0  = stream_.read_raw<u64>();
        u64 bits1  = stream_.read_raw<u16>();
        output[0]  |= ((bits0 & 0x1F)) << shift;
        output[1]  |= ((bits0 & (0x1Full <<  5)) >>  5) << shift;
        output[2]  |= ((bits0 & (0x1Full << 10)) >> 10) << shift;
        output[3]  |= ((bits0 & (0x1Full << 15)) >> 15) << shift;
        output[4]  |= ((bits0 & (0x1Full << 20)) >> 20) << shift;
        output[5]  |= ((bits0 & (0x1Full << 25)) >> 25) << shift;
        output[6]  |= ((bits0 & (0x1Full << 30)) >> 30) << shift;
        output[7]  |= ((bits0 & (0x1Full << 35)) >> 35) << shift;
        output[8]  |= ((bits0 & (0x1Full << 40)) >> 40) << shift;
        output[9]  |= ((bits0 & (0x1Full << 45)) >> 45) << shift;
        output[10] |= ((bits0 & (0x1Full << 50)) >> 50) << shift;
        output[11] |= ((bits0 & (0x1Full << 55)) >> 55) << shift;
        output[12] |= (((bits0 & (0x0Full << 60)) >> 60) | ((bits1 & 1) << 4)) << shift;
        output[13] |= ((bits1 & (0x1Full <<  1)) >>  1) << shift;
        output[14] |= ((bits1 & (0x1Full <<  6)) >>  6) << shift;
        output[15] |= ((bits1 & (0x1Full << 11)) >> 11) << shift;
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

    void _unpack6(u64* output, int shift) {
        u64 bits0  = stream_.read_raw<u64>();
        u64 bits1  = stream_.read_raw<u32>();
        output[0]  |= ((bits0 & 0x3F)) << shift;
        output[1]  |= ((bits0 & (0x3Full <<  6)) >>  6) << shift;
        output[2]  |= ((bits0 & (0x3Full << 12)) >> 12) << shift;
        output[3]  |= ((bits0 & (0x3Full << 18)) >> 18) << shift;
        output[4]  |= ((bits0 & (0x3Full << 24)) >> 24) << shift;
        output[5]  |= ((bits0 & (0x3Full << 30)) >> 30) << shift;
        output[6]  |= ((bits0 & (0x3Full << 36)) >> 36) << shift;
        output[7]  |= ((bits0 & (0x3Full << 42)) >> 42) << shift;
        output[8]  |= ((bits0 & (0x3Full << 48)) >> 48) << shift;
        output[9]  |= ((bits0 & (0x3Full << 54)) >> 54) << shift;
        output[10] |= (((bits0 & (0xFull << 60)) >> 60) | (bits1 & 0x3) << 2) << shift;
        output[11] |= ((bits1 & (0x3Full <<  2)) >>  2) << shift;
        output[12] |= ((bits1 & (0x3Full <<  8)) >>  8) << shift;
        output[13] |= ((bits1 & (0x3Full << 14)) >> 14) << shift;
        output[14] |= ((bits1 & (0x3Full << 20)) >> 20) << shift;
        output[15] |= ((bits1 & (0x3Full << 26)) >> 26) << shift;
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

    void _unpack7(u64* output, int shift) {
        u64 bits0  = stream_.read_raw<u64>();
        u64 bits1  = stream_.read_raw<u32>();
        u64 bits2  = stream_.read_raw<u16>();
        output[0]  |= ((bits0 & 0x7F)) << shift;
        output[1]  |= ((bits0 & (0x7Full <<  7)) >>  7) << shift;
        output[2]  |= ((bits0 & (0x7Full << 14)) >> 14) << shift;
        output[3]  |= ((bits0 & (0x7Full << 21)) >> 21) << shift;
        output[4]  |= ((bits0 & (0x7Full << 28)) >> 28) << shift;
        output[5]  |= ((bits0 & (0x7Full << 35)) >> 35) << shift;
        output[6]  |= ((bits0 & (0x7Full << 42)) >> 42) << shift;
        output[7]  |= ((bits0 & (0x7Full << 49)) >> 49) << shift;
        output[8]  |= ((bits0 & (0x7Full << 56)) >> 56) << shift;
        output[9]  |= (((bits0 & (0x01ull << 63)) >> 63) | ((bits1 & 0x3F) << 1)) << shift;
        output[10] |= ((bits1 & (0x7Full <<  6)) >>  6) << shift;
        output[11] |= ((bits1 & (0x7Full << 13)) >> 13) << shift;
        output[12] |= ((bits1 & (0x7Full << 20)) >> 20) << shift;
        output[13] |= (((bits1 & (0x1Full << 27)) >> 27) | ((bits2 & 0x03) << 5)) << shift;
        output[14] |= ((bits2 & (0x7Full <<  2)) >>  2) << shift;
        output[15] |= ((bits2 & (0x7Full <<  9)) >>  9) << shift;
    }

    template<typename T>
    void _shiftN(u64* input) {
        for (int i = 0; i < 16; i++) {
            input[i] >>= sizeof(T);
        }
    }

    bool pack(u64* input, int n) {
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
        case 25:
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack1(input);
        case 26:
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack2(input);
        case 27:
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack3(input);
        case 28:
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack4(input);
        case 29:
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack5(input);
        case 30:
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack6(input);
        case 31:
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack7(input);
        case 32:
            return _packN<u32>(input);
        case 33:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            return _pack1(input);
        case 34:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            return _pack2(input);
        case 35:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            return _pack3(input);
        case 36:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            return _pack4(input);
        case 37:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            return _pack5(input);
        case 38:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            return _pack6(input);
        case 39:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            return _pack7(input);
        case 40:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            return _packN<u8>(input);
        case 41:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack1(input);
        case 42:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack2(input);
        case 43:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack3(input);
        case 44:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack4(input);
        case 45:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack5(input);
        case 46:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack6(input);
        case 47:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack7(input);
        case 48:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            return _packN<u16>(input);
        case 49:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _pack1(input);
        case 50:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _pack2(input);
        case 51:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _pack3(input);
        case 52:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _pack4(input);
        case 53:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _pack5(input);
        case 54:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _pack6(input);
        case 55:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _pack7(input);
        case 56:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            return _packN<u8>(input);
        case 57:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack1(input);
        case 58:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack2(input);
        case 59:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack3(input);
        case 60:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack4(input);
        case 61:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack5(input);
        case 62:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack6(input);
        case 63:
            if (!_packN<u32>(input)) {
                return false;
            }
            _shiftN<u32>(input);
            if (!_packN<u16>(input)) {
                return false;
            }
            _shiftN<u16>(input);
            if (!_packN<u8>(input)) {
                return false;
            }
            _shiftN<u8>(input);
            return _pack7(input);
        case 64:
            return _packN<u64>(input);
        }
    }

    void unpack(u64* output, int n) {
        switch(n) {
        case 0:
            break;
        case 1:
            _unpack1(output, 0);
            break;
        case 2:
            _unpack2(output, 0);
            break;
        case 3:
            _unpack3(output, 0);
            break;
        case 4:
            _unpack3(output, 0);
            break;
        case 5:
            _unpack5(output, 0);
            break;
        case 6:
            _unpack6(output, 0);
            break;
        case 7:
            _unpack7(output, 0);
            break;
        case 8:
            _unpackN<u8>(output, 0);
            break;
        case 9:
            _unpackN<u8>(output, 0);
            _unpack1(output, 8);
            break;
        case 10:
            _unpackN<u8>(output, 0);
            _unpack2(output, 8);
            break;
        case 11:
            _unpackN<u8>(output, 0);
            _unpack3(output, 8);
            break;
        case 12:
            _unpackN<u8>(output, 0);
            _unpack4(output, 8);
            break;
        case 13:
            _unpackN<u8>(output, 0);
            _unpack5(output, 8);
            break;
        case 14:
            _unpackN<u8>(output, 0);
            _unpack6(output, 8);
            break;
        case 15:
            _unpackN<u8>(output, 0);
            _unpack7(output, 8);
            break;
        case 16:
            _unpackN<u16>(output, 0);
            break;
        case 17:
            _unpackN<u16>(output, 0);
            _unpack1(output, 16);
            break;
        case 18:
            _unpackN<u16>(output, 0);
            _unpack2(output, 16);
            break;
        case 19:
            _unpackN<u16>(output, 0);
            _unpack3(output, 16);
            break;
        case 20:
            _unpackN<u16>(output, 0);
            _unpack4(output, 16);
            break;
        case 21:
            _unpackN<u16>(output, 0);
            _unpack5(output, 16);
            break;
        case 22:
            _unpackN<u16>(output, 0);
            _unpack6(output, 16);
            break;
        case 23:
            _unpackN<u16>(output, 0);
            _unpack7(output, 16);
            break;
        case 24:
            _unpackN<u16>(output, 0);
            _unpackN<u8>(output, 16);
            break;
        case 25:
            _unpackN<u16>(output, 0);
            _unpackN<u8>(output, 16);
            _unpack1(output, 24);
            break;
        case 26:
            _unpackN<u16>(output, 0);
            _unpackN<u8>(output, 16);
            _unpack2(output, 24);
            break;
        case 27:
            _unpackN<u16>(output, 0);
            _unpackN<u8>(output, 16);
            _unpack3(output, 24);
            break;
        case 28:
            _unpackN<u16>(output, 0);
            _unpackN<u8>(output, 16);
            _unpack4(output, 24);
            break;
        case 29:
            _unpackN<u16>(output, 0);
            _unpackN<u8>(output, 16);
            _unpack5(output, 24);
            break;
        case 30:
            _unpackN<u16>(output, 0);
            _unpackN<u8>(output, 16);
            _unpack6(output, 24);
            break;
        case 31:
            _unpackN<u16>(output, 0);
            _unpackN<u8>(output, 16);
            _unpack7(output, 24);
            break;
        case 32:
            _unpackN<u32>(output, 0);
            break;
        case 33:
            _unpackN<u32>(output, 0);
            _unpack1(output, 32);
            break;
        case 34:
            _unpackN<u32>(output, 0);
            _unpack2(output, 32);
            break;
        case 35:
            _unpackN<u32>(output, 0);
            _unpack3(output, 32);
            break;
        case 36:
            _unpackN<u32>(output, 0);
            _unpack4(output, 32);
            break;
        case 37:
            _unpackN<u32>(output, 0);
            _unpack5(output, 32);
            break;
        case 38:
            _unpackN<u32>(output, 0);
            _unpack6(output, 32);
            break;
        case 39:
            _unpackN<u32>(output, 0);
            _unpack7(output, 32);
            break;
        case 40:
            _unpackN<u32>(output, 0);
            _unpackN<u8>(output, 32);
            break;
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
