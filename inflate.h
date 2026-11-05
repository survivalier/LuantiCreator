#pragma once
// Décompression « deflate » (RFC 1951) autonome, C++ standard uniquement.
// Basé sur l'algorithme de référence « puff » de Mark Adler (zlib), réécrit en C++.

#include <cstddef>
#include <cstdint>
#include <vector>

namespace lcinflate {

namespace detail {

constexpr int MAXBITS = 15;
constexpr int MAXLCODES = 286;
constexpr int MAXDCODES = 30;
constexpr int MAXCODES = MAXLCODES + MAXDCODES;
constexpr int FIXLCODES = 288;

struct Fail {};  // données invalides ou tronquées, ou sortie trop grande

struct Huffman {
    short count[MAXBITS + 1];
    short symbol[MAXCODES + 2];
};

struct State {
    const uint8_t *in;
    size_t inlen;
    size_t incnt = 0;
    int bitbuf = 0;
    int bitcnt = 0;
    std::vector<uint8_t> *out;
    size_t maxOut;
};

inline int bits(State &s, int need)
{
    long val = s.bitbuf;
    while (s.bitcnt < need) {
        if (s.incnt == s.inlen) throw Fail{};
        val |= long(s.in[s.incnt++]) << s.bitcnt;
        s.bitcnt += 8;
    }
    s.bitbuf = int(val >> need);
    s.bitcnt -= need;
    return int(val & ((1L << need) - 1));
}

inline void put(State &s, uint8_t b)
{
    if (s.out->size() >= s.maxOut) throw Fail{};
    s.out->push_back(b);
}

inline int decode(State &s, const Huffman &h)
{
    int code = 0, first = 0, index = 0;
    for (int len = 1; len <= MAXBITS; ++len) {
        code |= bits(s, 1);
        const int count = h.count[len];
        if (code - count < first) return h.symbol[index + (code - first)];
        index += count;
        first += count;
        first <<= 1;
        code <<= 1;
    }
    throw Fail{};
}

inline int construct(Huffman &h, const short *length, int n)
{
    short offs[MAXBITS + 1];
    for (int len = 0; len <= MAXBITS; ++len) h.count[len] = 0;
    for (int symbol = 0; symbol < n; ++symbol) h.count[length[symbol]]++;
    if (h.count[0] == n) return 0;
    int left = 1;
    for (int len = 1; len <= MAXBITS; ++len) {
        left <<= 1;
        left -= h.count[len];
        if (left < 0) return left;
    }
    offs[1] = 0;
    for (int len = 1; len < MAXBITS; ++len) offs[len + 1] = short(offs[len] + h.count[len]);
    for (int symbol = 0; symbol < n; ++symbol)
        if (length[symbol] != 0) h.symbol[offs[length[symbol]]++] = short(symbol);
    return left;
}

inline void codes(State &s, const Huffman &lencode, const Huffman &distcode)
{
    static const short lens[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27,
                                   31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    static const short lext[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                   2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    static const short dists[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129,
                                    193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097,
                                    6145, 8193, 12289, 16385, 24577};
    static const short dext[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6,
                                   6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
    int symbol;
    do {
        symbol = decode(s, lencode);
        if (symbol < 256) {
            put(s, uint8_t(symbol));
        } else if (symbol > 256) {
            symbol -= 257;
            if (symbol >= 29) throw Fail{};
            const int len = lens[symbol] + bits(s, lext[symbol]);
            const int ds = decode(s, distcode);
            if (ds >= 30) throw Fail{};
            const size_t dist = size_t(dists[ds] + bits(s, dext[ds]));
            if (dist > s.out->size()) throw Fail{};
            for (int i = 0; i < len; ++i) {
                const uint8_t b = (*s.out)[s.out->size() - dist];
                put(s, b);
            }
        }
    } while (symbol != 256);
}

inline void stored(State &s)
{
    s.bitbuf = 0;
    s.bitcnt = 0;
    if (s.incnt + 4 > s.inlen) throw Fail{};
    unsigned len = s.in[s.incnt++];
    len |= unsigned(s.in[s.incnt++]) << 8;
    const unsigned c0 = s.in[s.incnt++];
    const unsigned c1 = s.in[s.incnt++];
    if (c0 != (~len & 0xff) || c1 != ((~len >> 8) & 0xff)) throw Fail{};
    if (s.incnt + len > s.inlen) throw Fail{};
    for (unsigned i = 0; i < len; ++i) put(s, s.in[s.incnt++]);
}

inline void fixedBlock(State &s)
{
    static bool virgin = true;
    static Huffman lencode, distcode;
    if (virgin) {
        short lengths[FIXLCODES];
        int symbol = 0;
        for (; symbol < 144; ++symbol) lengths[symbol] = 8;
        for (; symbol < 256; ++symbol) lengths[symbol] = 9;
        for (; symbol < 280; ++symbol) lengths[symbol] = 7;
        for (; symbol < FIXLCODES; ++symbol) lengths[symbol] = 8;
        construct(lencode, lengths, FIXLCODES);
        for (symbol = 0; symbol < MAXDCODES; ++symbol) lengths[symbol] = 5;
        construct(distcode, lengths, MAXDCODES);
        virgin = false;
    }
    codes(s, lencode, distcode);
}

inline void dynamicBlock(State &s)
{
    static const short order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    short lengths[MAXCODES];
    Huffman lencode, distcode;

    const int nlen = bits(s, 5) + 257;
    const int ndist = bits(s, 5) + 1;
    const int ncode = bits(s, 4) + 4;
    if (nlen > MAXLCODES || ndist > MAXDCODES) throw Fail{};

    int index = 0;
    for (; index < ncode; ++index) lengths[order[index]] = short(bits(s, 3));
    for (; index < 19; ++index) lengths[order[index]] = 0;
    if (construct(lencode, lengths, 19) != 0) throw Fail{};

    index = 0;
    while (index < nlen + ndist) {
        int symbol = decode(s, lencode);
        if (symbol < 16) {
            lengths[index++] = short(symbol);
        } else {
            int len = 0;
            if (symbol == 16) {
                if (index == 0) throw Fail{};
                len = lengths[index - 1];
                symbol = 3 + bits(s, 2);
            } else if (symbol == 17) {
                symbol = 3 + bits(s, 3);
            } else {
                symbol = 11 + bits(s, 7);
            }
            if (index + symbol > nlen + ndist) throw Fail{};
            while (symbol--) lengths[index++] = short(len);
        }
    }
    if (lengths[256] == 0) throw Fail{};

    int err = construct(lencode, lengths, nlen);
    if (err && (err < 0 || nlen != lencode.count[0] + lencode.count[1])) throw Fail{};
    err = construct(distcode, lengths + nlen, ndist);
    if (err && (err < 0 || ndist != distcode.count[0] + distcode.count[1])) throw Fail{};
    codes(s, lencode, distcode);
}

}  // namespace detail

// Décompresse un flux deflate brut (sans en-tête zlib). Renvoie false si les données sont
// invalides/tronquées ou si la sortie dépasse maxOut octets.
inline bool inflateRaw(const uint8_t *in, size_t inlen, std::vector<uint8_t> &out, size_t maxOut)
{
    using namespace detail;
    State s;
    s.in = in;
    s.inlen = inlen;
    s.out = &out;
    s.maxOut = maxOut;
    try {
        int last;
        do {
            last = bits(s, 1);
            const int type = bits(s, 2);
            if (type == 0) stored(s);
            else if (type == 1) fixedBlock(s);
            else if (type == 2) dynamicBlock(s);
            else return false;
        } while (!last);
    } catch (const Fail &) {
        return false;
    }
    return true;
}

}  // namespace lcinflate
