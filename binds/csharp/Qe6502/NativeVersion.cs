using System;

namespace Qe6502
{
    internal static class NativeVersion
    {
        internal static readonly uint CompiledAbiVersionMajor;
        internal static readonly uint CompiledAbiVersionMinor;
        internal static readonly uint CompiledAbiVersion;

        static NativeVersion()
        {
            Version version = typeof(NativeVersion).Assembly.GetName().Version;
            CompiledAbiVersionMajor = (uint)(version == null ? 0 : version.Major);
            CompiledAbiVersionMinor = (uint)(version == null ? 0 : version.Minor);
            CompiledAbiVersion = (CompiledAbiVersionMajor << 16) | CompiledAbiVersionMinor;
        }
    }
}
