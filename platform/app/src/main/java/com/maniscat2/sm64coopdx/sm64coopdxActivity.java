package com.maniscat2.sm64coopdx;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.UriPermission;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.util.Log;

import org.libsdl.app.SDLActivity;

import java.util.List;

public class sm64coopdxActivity extends SDLActivity {
    private static final String TAG = "sm64coopdxActivity";
    private static final int REQUEST_CODE_SAF_TREE = 9001;

    public static native void nativeSetSafRomFd(int fd);
    public static native void nativeSetSafAssetFd(String relativePath, int fd);

    public static void requestChangeFolder() {
        if (mSingleton instanceof sm64coopdxActivity) {
            ((sm64coopdxActivity) mSingleton).launchSafPicker();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        List<UriPermission> permissions = getContentResolver().getPersistedUriPermissions();
        if (!permissions.isEmpty()) {
            Uri treeUri = permissions.get(0).getUri();
            if (treeUri != null) {
                traverseSafTree(treeUri);
                return;
            }
        }
        launchSafPicker();
    }

    public void launchSafPicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        startActivityForResult(intent, REQUEST_CODE_SAF_TREE);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_CODE_SAF_TREE && resultCode == Activity.RESULT_OK && data != null) {
            Uri treeUri = data.getData();
            if (treeUri != null) {
                try {
                    getContentResolver().takePersistableUriPermission(treeUri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
                } catch (Exception e) {
                    Log.e(TAG, "Failed to persist URI permission", e);
                }
                traverseSafTree(treeUri);
            }
        }
    }

    private void traverseSafTree(Uri treeUri) {
        String docId = DocumentsContract.getTreeDocumentId(treeUri);
        Uri rootDocUri = DocumentsContract.buildDocumentUriUsingTree(treeUri, docId);
        processDirectory(treeUri, docId, "");
    }

    private void processDirectory(Uri treeUri, String parentDocId, String currentPath) {
        ContentResolver resolver = getContentResolver();
        Uri childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(treeUri, parentDocId);

        String[] projection = new String[] {
            DocumentsContract.Document.COLUMN_DOCUMENT_ID,
            DocumentsContract.Document.COLUMN_DISPLAY_NAME,
            DocumentsContract.Document.COLUMN_MIME_TYPE
        };

        Cursor cursor = null;
        try {
            cursor = resolver.query(childrenUri, projection, null, null, null);
            if (cursor != null) {
                while (cursor.moveToNext()) {
                    String docId = cursor.getString(0);
                    String name = cursor.getString(1);
                    String mimeType = cursor.getString(2);

                    if (name == null || docId == null) continue;

                    String relativePath = currentPath.isEmpty() ? name : currentPath + "/" + name;

                    if (DocumentsContract.Document.MIME_TYPE_DIR.equals(mimeType)) {
                        processDirectory(treeUri, docId, relativePath);
                    } else {
                        try {
                            Uri fileUri = DocumentsContract.buildDocumentUriUsingTree(treeUri, docId);
                            ParcelFileDescriptor pfd = resolver.openFileDescriptor(fileUri, "r");
                            if (pfd != null) {
                                int fd = pfd.detachFd();
                                if (name.endsWith(".z64") || name.endsWith(".n64") || name.endsWith(".v64")) {
                                    nativeSetSafRomFd(fd);
                                } else {
                                    nativeSetSafAssetFd(relativePath, fd);
                                }
                            }
                        } catch (Exception e) {
                            Log.e(TAG, "Failed to open FD for " + relativePath, e);
                        }
                    }
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to query directory " + parentDocId, e);
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
    }
}
