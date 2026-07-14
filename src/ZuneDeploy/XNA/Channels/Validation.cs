namespace ZuneDeploy.XNA.Channels;

internal static class ChannelValidation {
    internal const int MaxComponentLength = 40;
    internal const int MaxPathLength = 247;

    internal static void ValidateFilePath(string path) {
        if (path.Length > MaxPathLength) {
            throw new Exception($"File path length {path.Length} exceeds device maximum of {MaxPathLength}");
        }
        if (path.Length == 0 || path[^1] == '\\') {
            throw new Exception("File path must end in a file name");
        }
        foreach (string component in path.Split('\\')) {
            if (component.Length > MaxComponentLength) {
                throw new Exception($"Path component '{component}' exceeds device maximum name length of {MaxComponentLength}");
            }
        }
        foreach (char c in path) {
            if (c >= '\u0080' || c == '/') {
                throw new Exception($"Invalid Character {c} in file path");
            }
        }
    }

    internal static void ValidateFileStream(Stream stream) {
        if (stream.Length > int.MaxValue) {
            throw new Exception("File exceeds maximum size of 2 GB");
        }
    }
}