# 🔐 CryptoShredder

A simple C utility to **securely encrypt and destroy files or directories**, making data irrecoverable without the encryption key. This tool is designed to work effectively even on SSDs, where traditional overwriting methods are unreliable due to wear leveling and internal caching.

---

## 🚀 Features

* 🔒 **Encrypts files using AES-256-CBC**
* 💥 **Destroys the encryption key immediately after use**
* 🧠 **No key stored on disk** — once encryption is complete, data cannot be recovered
* 🗂️ Works recursively on directories
* 🗑️ Optional deletion of the original file or directory after encryption
* ✅ SSD-friendly: does not rely on physical overwriting

---

## 📦 Requirements

You must have OpenSSL development libraries installed:

```bash
sudo apt install libssl-dev
```

---

## 🛠️ Compilation

Compile with:

```bash
gcc -o cryptoshredder cryptoshredder.c -lcrypto
```

---

## 🧪 Usage

```bash
./cryptoshredder -f <file>         # Encrypt a single file
./cryptoshredder -d <directory>    # Encrypt all files in a directory recursively

# Optional: delete files/directories after encryption
./cryptoshredder -frm <file>       # Encrypt and delete file
./cryptoshredder -drm <directory>  # Encrypt and delete directory
```

---

## 🔐 How it Works

* The tool reads the file into memory.
* It generates a random **AES-256 key and IV** using `/dev/urandom`.
* It encrypts the contents in-place using **OpenSSL's AES-256-CBC**.
* It **overwrites the original file** with the encrypted version.
* The encryption key and IV are securely wiped from memory using a `secure_zero()` function.
* Optionally, it deletes the original file or folder using `remove()` or `rmdir()`.

⚠️ Once the key is destroyed, **decryption is impossible**.

---

## ⚠️ Warning

This tool performs **irreversible encryption**. There is no backup of the encryption key. Use it only when you are sure the data must be rendered permanently inaccessible.

---

## ✅ Why Use Encryption Instead of Overwriting?

Traditional shredding tools overwrite files to make recovery impossible. However, on **SSDs**, overwriting isn't guaranteed due to wear-leveling and internal remapping. This tool avoids that problem by **cryptographically destroying access** to the data — rendering it useless even if it's physically intact.


