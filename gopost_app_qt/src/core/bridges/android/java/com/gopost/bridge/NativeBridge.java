package com.gopost.bridge;

/**
 * NativeBridge — Java companion for AndroidBridge (JNI).
 *
 * Each static native method maps 1:1 to an IPlatformBridge method.
 * The C++ side (AndroidBridge) calls these via JNI, and the Java side
 * delegates to Android framework APIs (ContentResolver, Context, etc.).
 *
 * Usage:
 *   System.loadLibrary("gopost_bridge");
 *   // Native methods become available after JNI_OnLoad runs.
 */
public class NativeBridge {

    // Load the native library
    static {
        System.loadLibrary("gopost_bridge");
    }

    // ---- File Access ----

    /** Launch ACTION_OPEN_DOCUMENT intent and return selected URIs. */
    public static native String[] showFilePicker(String[] allowedExtensions, boolean allowMultiple, String title);

    /** Read bytes from a file path or content:// URI. */
    public static native byte[] readFileBytes(String pathOrUri, long offset, long length);

    /** Get file metadata (name, size, MIME, lastModified, isReadOnly) as JSON. */
    public static native String getFileMetadata(String pathOrUri);

    /** Copy a source URI to the app's cache directory. Returns the cache path. */
    public static native String copyToAppCache(String sourceUri, String destFilename);

    /** Returns Context.getCacheDir() + "/gopost". */
    public static native String getAppCacheDirectory();

    /** Returns Context.getExternalFilesDir(DIRECTORY_DOCUMENTS). */
    public static native String getAppDocumentsDirectory();

    // ---- System Info ----

    /** Returns Build.MODEL. */
    public static native String getDeviceModel();

    /** Returns "Android " + Build.VERSION.RELEASE + " (API " + SDK_INT + ")". */
    public static native String getOsVersionString();

    /** Returns StatFs.getAvailableBytes() for the data directory. */
    public static native long getAvailableStorageBytes();

    /** Returns Context.getCacheDir() + "/tmp". */
    public static native String getTempDirectory();
}
