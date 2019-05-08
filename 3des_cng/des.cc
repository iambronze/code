#include "des.h"
#include <string.h>
#include <memory>

#pragma comment(lib, "Bcrypt.lib")

namespace crypto {

namespace {
const int DES_BLOCK_SIZE = 8;
}

Crypto3DesCNG::Crypto3DesCNG(Padding padding)
    : padding_(padding),
      block_size_(DES_BLOCK_SIZE),
      alg_handle_(nullptr) {}

Crypto3DesCNG::~Crypto3DesCNG() {
  Destroy();
}

void Crypto3DesCNG::Destroy() {
  if (alg_handle_) {
    BCryptCloseAlgorithmProvider(alg_handle_, 0);
    alg_handle_ = nullptr;
  }
}

bool Crypto3DesCNG::Initialize() {
  NTSTATUS status = BCryptOpenAlgorithmProvider(&alg_handle_,
                                                BCRYPT_3DES_ALGORITHM,
                                                nullptr,
                                                0);
  if (!BCRYPT_SUCCESS(status)) {
    return false;
  }

  status = BCryptSetProperty(alg_handle_,
                             BCRYPT_CHAINING_MODE,
                             (PBYTE) BCRYPT_CHAIN_MODE_ECB,
                             sizeof(BCRYPT_CHAIN_MODE_ECB),
                             0);
  if (!BCRYPT_SUCCESS(status)) {
    Destroy();
    return false;
  }

  DWORD size = 0;
  status = BCryptGetProperty(alg_handle_,
                             BCRYPT_BLOCK_LENGTH,
                             (PUCHAR) &block_size_,
                             sizeof(DWORD),
                             &size,
                             0);
  if (!BCRYPT_SUCCESS(status)) {
    Destroy();
    return false;
  }
  return true;
}

BCRYPT_KEY_HANDLE Crypto3DesCNG::CreateKeyHandle(const std::string &key) {
  size_t key_count = key.size() / DES_BLOCK_SIZE;
  if (key.size() % DES_BLOCK_SIZE)
    key_count += 1;
  if (key_count < 3)
    key_count = 3;
  size_t key_size = key_count * DES_BLOCK_SIZE;

  auto deskey = std::make_unique<uint8_t[]>(key_size);
  memset(deskey.get(), 0, key_count);
  memcpy_s(deskey.get(), key_size, key.data(), key.size());
  BCRYPT_KEY_HANDLE key_handle = nullptr;
  NTSTATUS status = BCryptGenerateSymmetricKey(alg_handle_,
                                               &key_handle,
                                               NULL,
                                               0,
                                               (PBYTE) (deskey.get()),
                                               key_size,
                                               0);
  if (!BCRYPT_SUCCESS(status)) {
    return NULL;
  }
  return key_handle;
}

bool Crypto3DesCNG::DoEncrypt(const std::string &key,
                              const std::string &plaintext,
                              std::string *ciphertext) {
  if (alg_handle_ == NULL)
    return false;
  BCRYPT_KEY_HANDLE key_handle = CreateKeyHandle(key);
  if (key_handle == NULL)
    return false;

  int pad = plaintext.size() % block_size_;
  size_t buffer_size = plaintext.size();
  if (padding_ == PKCS5 || pad > 0) {
    buffer_size += (block_size_ - pad);
  }
  std::unique_ptr<uint8_t[]> source(new uint8_t[buffer_size]);
  memset(source.get(), 0, buffer_size);
  memcpy_s(source.get(), buffer_size, plaintext.data(), plaintext.size());

  if (padding_ == PKCS5) {
    for (int i = plaintext.size(); i < buffer_size; i++) {
      source[i] = pad ? (block_size_ - pad) : 0x08;
    }
  }

  DWORD size;
  NTSTATUS status = BCryptEncrypt(key_handle,
                                  (PUCHAR) source.get(),
                                  buffer_size,
                                  nullptr,
                                  nullptr,
                                  block_size_,
                                  nullptr,
                                  0,
                                  &size,
                                  0);
  if (!BCRYPT_SUCCESS(status)) {
    BCryptDestroyKey(key_handle);
    return false;
  }
  auto output = std::make_unique<UCHAR[]>(size);
  status = BCryptEncrypt(key_handle,
                         (PUCHAR) source.get(),
                         buffer_size,
                         nullptr,
                         nullptr,
                         block_size_,
                         &output[0],
                         size,
                         &size,
                         0);
  BCryptDestroyKey(key_handle);
  if (!BCRYPT_SUCCESS(status)) {
    return false;
  }
  ciphertext->clear();
  ciphertext->append((char *) output.get(), size);
  return true;
}

bool Crypto3DesCNG::DoDecrypt(const std::string &key,
                              const std::string &ciphertext,
                              std::string *plaintext) {

  if (alg_handle_ == NULL)
    return false;

  BCRYPT_KEY_HANDLE key_handle = CreateKeyHandle(key);
  if (key_handle == NULL)
    return false;

  size_t cipher_size = ciphertext.size();

  std::unique_ptr<uint8_t[]> source(new uint8_t[cipher_size]);
  memset(source.get(), 0, cipher_size);
  memcpy_s(source.get(), cipher_size, ciphertext.data(), cipher_size);

  DWORD size;
  NTSTATUS status = BCryptDecrypt(key_handle,
                                  (PUCHAR) source.get(),
                                  cipher_size,
                                  nullptr,
                                  nullptr,
                                  block_size_,
                                  nullptr,
                                  0,
                                  &size,
                                  0);
  if (!BCRYPT_SUCCESS(status)) {
    BCryptDestroyKey(key_handle);
    return false;
  }
  auto output = std::make_unique<UCHAR[]>(size);
  status = BCryptDecrypt(key_handle,
                         (PUCHAR) source.get(),
                         cipher_size,
                         nullptr,
                         nullptr,
                         block_size_,
                         &output[0],
                         size,
                         &size,
                         0);
  BCryptDestroyKey(key_handle);

  if (!BCRYPT_SUCCESS(status)) {
    return false;
  }
  plaintext->clear();

  uint8_t unpad = 0;
  if (padding_ == PKCS5) {
    unpad = output[size - 1];
    if (unpad > cipher_size)
      unpad = 0;
  } else {
    for (int i = size - 1; i > 0; i--) {
      if (output[i] > 0)
        break;
      ++unpad;
    }
  }
  plaintext->append(output.get(), output.get() + size - unpad);
  return true;
}

} //cryto