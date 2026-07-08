using System.Runtime.InteropServices;
using System.Runtime.InteropServices.Marshalling;

namespace ZuneDeploy.Transport;

static internal partial class MTP {
    [LibraryImport("zune-deploy-native")]
    internal static partial int OpenConnection(out IntPtr device);

    [LibraryImport("zune-deploy-native")]

    internal static partial void CloseConnection(IntPtr device);

    [LibraryImport("zune-deploy-native")]
    internal static partial int PollData(IntPtr device, [MarshalUsing(CountElementName = "size")] byte[] buffer, int size, out int bytesRead);

    [LibraryImport("zune-deploy-native")]
    internal static partial int SendData(IntPtr device, [MarshalUsing(CountElementName = "size")] byte[] buffer, int size);

    [LibraryImport("zune-deploy-native")]
    internal static partial byte GetDeviceFamily(IntPtr device);
}
