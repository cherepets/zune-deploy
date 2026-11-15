using System.Runtime.InteropServices;
using System.Runtime.InteropServices.Marshalling;
using NativeGen;

namespace ZuneDeploy.Transport;

static internal partial class MTP {
    // TODO: Remove together with previous api
    private static readonly bool TempFF = false;

    internal static int OpenConnection(out IntPtr device) {
        if (TempFF) {
            return Legacy.OpenConnection(out device);
        }

        device = IntPtr.Zero;
        var result = ZuneConnection.Open(out var connection);
        if (result == Result.Ok) {
            device = GCHandle.ToIntPtr(GCHandle.Alloc(connection));
        }
        return (int)result;
    }

    internal static void CloseConnection(IntPtr device) {
        if (TempFF) {
            Legacy.CloseConnection(device);
            return;
        }

        var handle = GCHandle.FromIntPtr(device);
        Get(device).Close();
        handle.Free();
    }

    internal static int PollData(IntPtr device, byte[] buffer, int size, out int bytesRead) {
        if (TempFF) {
            return Legacy.PollData(device, buffer, size, out bytesRead);
        }
        return (int)Get(device).PollData(buffer.AsSpan(0, size), out bytesRead);
    }

    internal static int SendData(IntPtr device, byte[] buffer, int size) {
        if (TempFF) {
            return Legacy.SendData(device, buffer, size);
        }
        return (int)Get(device).SendData(buffer.AsSpan(0, size));
    }

    internal static byte GetDeviceFamily(IntPtr device) {
        if (TempFF) {
            return Legacy.GetDeviceFamily(device);
        }
        return Get(device).DeviceFamily;
    }

    private static ZuneConnection Get(IntPtr device) {
        return (ZuneConnection)GCHandle.FromIntPtr(device).Target!;
    }

    // TODO: Remove together with previous api
    private static partial class Legacy {
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
}
