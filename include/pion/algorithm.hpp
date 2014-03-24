// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_ALGORITHM_HEADER__
#define __PION_ALGORITHM_HEADER__

#include <string>
#include <boost/cstdint.hpp>
#include <pion/config.hpp>

namespace pion {    // begin namespace pion

struct PION_API algorithm {

    /** base64 decoding
     *
     * @param input - base64 encoded string
     * @param output - decoded string ( may include non-text chars)
     * @return true if successful, false if input string contains non-base64 symbols
     */
    static bool base64_decode(std::string const &input, std::string & output);

    /** base64 encoding
     *
     * @param input - arbitrary string ( may include non-text chars)
     * @param output - base64 encoded string
     * @return true if successful,
     */
    static bool base64_encode(std::string const &input, std::string & output);

    /// escapes URL-encoded strings (a%20value+with%20spaces)
    static std::string url_decode(const std::string& str);

    /// encodes strings so that they are safe for URLs (with%20spaces)
    static std::string url_encode(const std::string& str);

    /// TODO: escapes XML/HTML-encoded strings (1 &lt; 2)
    //static std::string xml_decode(const std::string& str);
    
    /// encodes strings so that they are safe for XML/HTML (2 &gt; 1)
    static std::string xml_encode(const std::string& str);
    
    /// convert sequence of bytes in IEEE 754 format into a native floating point data type
    /// reference: http://en.wikipedia.org/wiki/Single_precision_floating-point_format
    static void float_from_bytes(long double& value, const unsigned char *ptr, size_t num_exp_bits, size_t num_fraction_bits);

    /// convert native floating point type into a sequence of bytes in IEEE 754 format
    /// reference: http://en.wikipedia.org/wiki/Single_precision_floating-point_format
    static void float_to_bytes(long double value, unsigned char *ptr, size_t num_exp_bits, size_t num_fraction_bits);

    /// convert sequence of one byte to 8-bit unsigned integer
    static inline boost::uint8_t to_uint8(unsigned char byte) {
        return boost::uint8_t(byte);
    }
    
    /// convert sequence of one byte to 8-bit signed integer
    static inline boost::int8_t to_int8(unsigned char byte) {
        return boost::int8_t(byte);
    }
    
    /// convert sequence of one byte to 8-bit unsigned integer
    static inline boost::uint8_t to_uint8(char byte) {
        return boost::uint8_t(byte);
    }
    
    /// convert sequence of one byte to 8-bit signed integer
    static inline boost::int8_t to_int8(char byte) {
        return boost::int8_t(byte);
    }
    
    /// convert sequence of two bytes to 16-bit unsigned integer
    static inline boost::uint16_t to_uint16(unsigned char high, unsigned char low) {
        return (((boost::uint16_t)high) << 8) | ((boost::uint16_t)low);
    }

    /// convert sequence of two bytes to 16-bit signed integer
    static inline boost::int16_t to_int16(unsigned char high, unsigned char low) {
        return (((boost::int16_t)high) << 8) | ((boost::int16_t)low);
    }

    /// convert sequence of three bytes to 24-bit unsigned integer
    static inline boost::uint32_t to_uint24(unsigned char high, unsigned char mid, unsigned char low) {
        return (((boost::uint32_t)high) << 16) | (((boost::uint32_t)mid) << 8) | ((boost::uint32_t)low);
    }
    
    /// convert sequence of three bytes to 24-bit signed integer
    static inline boost::int32_t to_int24(unsigned char high, unsigned char mid, unsigned char low) {
        return (((boost::int32_t)high) << 16) | (((boost::int32_t)mid) << 8) | ((boost::int32_t)low);
    }
    
    /// convert sequence of four bytes to 32-bit unsigned integer
    static inline boost::uint32_t to_uint32(unsigned char high, unsigned char mid1, unsigned char mid2, unsigned char low) {
        return (((boost::uint32_t)high) << 24) | (((boost::uint32_t)mid1) << 16) | (((boost::uint32_t)mid2) << 8) | ((boost::uint32_t)low);
    }
    
    /// convert sequence of four bytes to 32-bit signed integer
    static inline boost::int32_t to_int32(unsigned char high, unsigned char mid1, unsigned char mid2, unsigned char low) {
        return (((boost::int32_t)high) << 24) | (((boost::int32_t)mid1) << 16) | (((boost::int32_t)mid2) << 8) | ((boost::int32_t)low);
    }
    
    /// convert sequence of eight bytes to 64-bit unsigned integer
    static inline boost::uint64_t to_uint64(unsigned char high, unsigned char mid1, unsigned char mid2, unsigned char mid3, unsigned char mid4, unsigned char mid5, unsigned char mid6, unsigned char low) {
        return (((boost::uint64_t)high) << 56) | (((boost::uint64_t)mid1) << 48) | (((boost::uint64_t)mid2) << 40) | (((boost::uint64_t)mid3) << 32)
            | (((boost::uint64_t)mid4) << 24) | (((boost::uint64_t)mid5) << 16) | (((boost::uint64_t)mid6) << 8) | ((boost::uint64_t)low);
    }
    
    /// convert sequence of eight bytes to 64-bit signed integer
    static inline boost::int64_t to_int64(unsigned char high, unsigned char mid1, unsigned char mid2, unsigned char mid3, unsigned char mid4, unsigned char mid5, unsigned char mid6, unsigned char low) {
        return (((boost::int64_t)high) << 56) | (((boost::int64_t)mid1) << 48) | (((boost::int64_t)mid2) << 40) | (((boost::int64_t)mid3) << 32)
        | (((boost::int64_t)mid4) << 24) | (((boost::int64_t)mid5) << 16) | (((boost::int64_t)mid6) << 8) | ((boost::int64_t)low);
    }

    /// convert sequence of two bytes to 16-bit unsigned integer
    template <typename T1, typename T2>
    static inline boost::uint16_t to_uint16(T1 high, T2 low) {
        return to_uint16(static_cast<unsigned char>(high), static_cast<unsigned char>(low));
    }
    
    /// convert sequence of two bytes to 16-bit signed integer
    template <typename T1, typename T2>
    static inline boost::int16_t to_int16(T1 high, T2 low) {
        return to_int16(static_cast<unsigned char>(high), static_cast<unsigned char>(low));
    }
    
    /// convert sequence of three bytes to 24-bit unsigned integer
    template <typename T1, typename T2, typename T3>
    static inline boost::uint32_t to_uint24(T1 high, T2 mid, T3 low) {
        return to_uint24(static_cast<unsigned char>(high),
                         static_cast<unsigned char>(mid),
                         static_cast<unsigned char>(low));
    }
    
    /// convert sequence of three bytes to 24-bit signed integer
    template <typename T1, typename T2, typename T3>
    static inline boost::int32_t to_int24(T1 high, T2 mid, T3 low) {
        return to_int24(static_cast<unsigned char>(high),
                        static_cast<unsigned char>(mid),
                        static_cast<unsigned char>(low));
    }
    
    /// convert sequence of four bytes to 32-bit unsigned integer
    template <typename T1, typename T2, typename T3, typename T4>
    static inline boost::uint32_t to_uint32(T1 high, T2 mid1, T3 mid2, T4 low) {
        return to_uint32(static_cast<unsigned char>(high),
                         static_cast<unsigned char>(mid1),
                         static_cast<unsigned char>(mid2),
                         static_cast<unsigned char>(low));
    }
    
    /// convert sequence of four bytes to 32-bit signed integer
    template <typename T1, typename T2, typename T3, typename T4>
    static inline boost::int32_t to_int32(T1 high, T2 mid1, T3 mid2, T4 low) {
        return to_int32(static_cast<unsigned char>(high),
                        static_cast<unsigned char>(mid1),
                        static_cast<unsigned char>(mid2),
                        static_cast<unsigned char>(low));
    }
    
    /// convert sequence of eight bytes to 64-bit unsigned integer
    template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    static inline boost::uint64_t to_uint64(T1 high, T2 mid1, T3 mid2, T4 mid3, T5 mid4, T6 mid5, T7 mid6, T8 low) {
        return to_uint64(static_cast<unsigned char>(high),
                         static_cast<unsigned char>(mid1),
                         static_cast<unsigned char>(mid2),
                         static_cast<unsigned char>(mid3),
                         static_cast<unsigned char>(mid4),
                         static_cast<unsigned char>(mid5),
                         static_cast<unsigned char>(mid6),
                         static_cast<unsigned char>(low));
    }
    
    /// convert sequence of eight bytes to 64-bit signed integer
    template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    static inline boost::int64_t to_int64(T1 high, T2 mid1, T3 mid2, T4 mid3, T5 mid4, T6 mid5, T7 mid6, T8 low) {
        return to_int64(static_cast<unsigned char>(high),
                        static_cast<unsigned char>(mid1),
                        static_cast<unsigned char>(mid2),
                        static_cast<unsigned char>(mid3),
                        static_cast<unsigned char>(mid4),
                        static_cast<unsigned char>(mid5),
                        static_cast<unsigned char>(mid6),
                        static_cast<unsigned char>(low));
    }
    
    
    /// convert byte pointer into an 8-bit unsigned integer
    template <typename Byte>
    static inline boost::uint8_t to_uint8(const Byte *buf) {
        return to_uint8(buf[0]);
    }
    
    /// convert byte pointer into an 8-bit signed integer
    template <typename Byte>
    static inline boost::int8_t to_int8(const Byte *buf) {
        return to_int8(buf[0]);
    }
    
    /// convert sequence of two bytes to 16-bit unsigned integer
    template <typename Byte>
    static inline boost::uint16_t to_uint16(const Byte *buf) {
        return to_uint16(buf[0], buf[1]);
    }
    
    /// convert sequence of two bytes to 16-bit signed integer
    template <typename Byte>
    static inline boost::int16_t to_int16(const Byte *buf) {
        return to_int16(buf[0], buf[1]);
    }
    
    /// convert sequence of three bytes to 24-bit unsigned integer
    template <typename Byte>
    static inline boost::uint32_t to_uint24(const Byte *buf) {
        return to_uint24(buf[0], buf[1], buf[2]);
    }
    
    /// convert sequence of three bytes to 24-bit signed integer
    template <typename Byte>
    static inline boost::int32_t to_int24(const Byte *buf) {
        return to_int24(buf[0], buf[1], buf[2]);
    }
    
    /// convert sequence of four bytes to 32-bit unsigned integer
    template <typename Byte>
    static inline boost::uint32_t to_uint32(const Byte *buf) {
        return to_uint32(buf[0], buf[1], buf[2], buf[3]);
    }
    
    /// convert sequence of four bytes to 32-bit signed integer
    template <typename Byte>
    static inline boost::int32_t to_int32(const Byte *buf) {
        return to_int32(buf[0], buf[1], buf[2], buf[3]);
    }
    
    /// convert sequence of eight bytes to 64-bit unsigned integer
    template <typename Byte>
    static inline boost::uint64_t to_uint64(const Byte *buf) {
        return to_uint64(buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
    }
    
    /// convert sequence of eight bytes to 64-bit signed integer
    template <typename Byte>
    static inline boost::int64_t to_int64(const Byte *buf) {
        return to_int64(buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
    }

    /// convert 8-bit unsigned integer into sequence of one byte
    template <typename Byte>
    static inline void from_uint8(Byte *buf, const boost::uint8_t n) {
        buf[0] = n & 0xFF;
    }
    
    /// convert 8-bit signed integer into sequence of one byte
    template <typename Byte>
    static inline void from_int8(Byte *buf, const boost::int8_t n) {
        buf[0] = n & 0xFF;
    }
    
    /// convert 16-bit unsigned integer into sequence of two bytes
    template <typename Byte>
    static inline void from_uint16(Byte *buf, const boost::uint16_t n) {
        buf[0] = (n >> 8) & 0xFF;
        buf[1] = n & 0xFF;
    }

    /// convert 16-bit signed integer into sequence of two bytes
    template <typename Byte>
    static inline void from_int16(Byte *buf, const boost::int16_t n) {
        buf[0] = (n >> 8) & 0xFF;
        buf[1] = n & 0xFF;
    }
    
    /// convert 24-bit unsigned integer into sequence of three bytes
    template <typename Byte>
    static inline void from_uint24(Byte *buf, const boost::uint32_t n) {
        buf[0] = (n >> 16) & 0xFF;
        buf[1] = (n >> 8) & 0xFF;
        buf[2] = n & 0xFF;
    }
    
    /// convert 24-bit signed integer into sequence of three bytes
    template <typename Byte>
    static inline void from_int24(Byte *buf, const boost::int32_t n) {
        buf[0] = (n >> 16) & 0xFF;
        buf[1] = (n >> 8) & 0xFF;
        buf[2] = n & 0xFF;
    }
    
    /// convert 32-bit unsigned integer into sequence of four bytes
    template <typename Byte>
    static inline void from_uint32(Byte *buf, const boost::uint32_t n) {
        buf[0] = (n >> 24) & 0xFF;
        buf[1] = (n >> 16) & 0xFF;
        buf[2] = (n >> 8) & 0xFF;
        buf[3] = n & 0xFF;
    }
    
    /// convert 32-bit signed integer into sequence of four bytes
    template <typename Byte>
    static inline void from_int32(Byte *buf, const boost::int32_t n) {
        buf[0] = (n >> 24) & 0xFF;
        buf[1] = (n >> 16) & 0xFF;
        buf[2] = (n >> 8) & 0xFF;
        buf[3] = n & 0xFF;
    }

    /// convert 64-bit unsigned integer into sequence of eight bytes
    template <typename Byte>
    static inline void from_uint64(Byte *buf, const boost::uint64_t n) {
        buf[0] = (n >> 56) & 0xFF;
        buf[1] = (n >> 48) & 0xFF;
        buf[2] = (n >> 40) & 0xFF;
        buf[3] = (n >> 32) & 0xFF;
        buf[4] = (n >> 24) & 0xFF;
        buf[5] = (n >> 16) & 0xFF;
        buf[6] = (n >> 8) & 0xFF;
        buf[7] = n & 0xFF;
    }
    
    /// convert 64-bit signed integer into sequence of eight bytes
    template <typename Byte>
    static inline void from_int64(Byte *buf, const boost::int64_t n) {
        buf[0] = (n >> 56) & 0xFF;
        buf[1] = (n >> 48) & 0xFF;
        buf[2] = (n >> 40) & 0xFF;
        buf[3] = (n >> 32) & 0xFF;
        buf[4] = (n >> 24) & 0xFF;
        buf[5] = (n >> 16) & 0xFF;
        buf[6] = (n >> 8) & 0xFF;
        buf[7] = n & 0xFF;
    }
    
    /// convert sequence of four bytes in 32-bit "single precision" binary32 format into a float
    /// http://en.wikipedia.org/wiki/Single_precision_floating-point_format
    template <typename Byte>
    static inline float to_float(const Byte *ptr) {
        long double value;
        float_from_bytes(value, (unsigned char *)ptr, 8U, 23U);
        return value;
    }
    
    /// convert sequence of eight bytes in 64-bit "double precision" binary64 format into a double
    /// http://en.wikipedia.org/wiki/Double_precision_floating-point_format
    template <typename Byte>
    static inline double to_double(const Byte *ptr) {
        long double value;
        float_from_bytes(value, (unsigned char *)ptr, 11U, 52U);
        return value;
    }
    
    /// convert sequence of sixteen bytes in 128-bit "quadruple precision" format into a long double
    /// http://en.wikipedia.org/wiki/Quadruple_precision_floating-point_format
    template <typename Byte>
    static inline long double to_long_double(const Byte *ptr) {
        long double value;
        float_from_bytes(value, (unsigned char *)ptr, 15U, 112U);
        return value;
    }
    
    /// convert float into sequence of four bytes in "single precision" binary32 format
    /// http://en.wikipedia.org/wiki/Single_precision_floating-point_format
    template <typename Byte>
    static inline void from_float(Byte *ptr, const float n) {
        float_to_bytes(n, (unsigned char*)ptr, 8U, 23U);
    }

    /// convert double into sequence of eight bytes in "double precision" binary64 format
    /// http://en.wikipedia.org/wiki/Double_precision_floating-point_format
    template <typename Byte>
    static inline void from_double(Byte *ptr, const double n) {
        float_to_bytes(n, (unsigned char*)ptr, 11U, 52U);
    }

    /// convert long double into sequence of sixteen bytes in 128-bit "quadruple precision" format
    /// http://en.wikipedia.org/wiki/Quadruple_precision_floating-point_format
    template <typename Byte>
    static inline void from_long_double(Byte *ptr, const long double n) {
        float_to_bytes(n, (unsigned char*)ptr, 15U, 112U);
    }
};
    
}   // end namespace pion

#endif
