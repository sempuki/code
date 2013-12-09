using System;
using System.Text;
using System.Security.Cryptography;

namespace ClipClient
{
    class Util
    {
        public static UInt64 GenerateRand64()
        {
            var rand = new Random();
            var buffer = new byte[sizeof(Int64)];
            rand.NextBytes(buffer);
            return BitConverter.ToUInt64(buffer, 0);
        }

        public static UInt64 GenerateRand63()
        {
            return GenerateRand64() & 0x7FFFFFFFFFFFFFFF;
        }

        public static UInt64 GenerateHash64(String phrase)
        {
            var input = Encoding.UTF8.GetBytes(phrase);
            var output = new SHA256CryptoServiceProvider().ComputeHash(input);
            ulong result =
                BitConverter.ToUInt64(output, 0 * sizeof(ulong)) ^
                BitConverter.ToUInt64(output, 1 * sizeof(ulong)) ^
                BitConverter.ToUInt64(output, 2 * sizeof(ulong)) ^
                BitConverter.ToUInt64(output, 3 * sizeof(ulong));
            return BitConverter.IsLittleEndian ? EndianSwap(result) : result;
        }

        public static UInt64 GenerateHash63(String phrase)
        {
            return GenerateHash64(phrase) & 0x7FFFFFFFFFFFFFFF;
        }

        public static UInt64 EndianSwap(UInt64 value)
        {
            return ((value & 0xFF00000000000000) >> 56) |
                   ((value & 0x00FF000000000000) >> 40) |
                   ((value & 0x0000FF0000000000) >> 24) |
                   ((value & 0x000000FF00000000) >>  8) |
                   ((value & 0x00000000FF000000) <<  8) |
                   ((value & 0x0000000000FF0000) << 24) |
                   ((value & 0x000000000000FF00) << 40) |
                   ((value & 0x00000000000000FF) << 56);
        }
    }
}
