#include <string.h>
#include <memory>
#include "pch.h"
#include "des.h"

namespace crypto {

const int DES_BLOCK_SIZE = 8;

bool SetBuffer(const char *data, size_t size,
               ABI::Windows::Storage::Streams::IBufferFactory *factory,
               ABI::Windows::Storage::Streams::IBuffer **out_buffer) {
  Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IBuffer> buffer;
  HRESULT hr = factory->Create(size, &buffer);
  if (hr != S_OK)
    return false;

  hr = buffer->put_Length(size);
  if (hr != S_OK)
    return false;

  ComPtr<Windows::Storage::Streams::IBufferByteAccess> buffer_access;
  hr = buffer.As(&buffer_access);
  if (hr != S_OK)
    return false;

  byte *bytes;
  hr = buffer_access->Buffer(&bytes);
  if (hr != S_OK)
    return false;

  memcpy(bytes, data, size);
  *out_buffer = buffer.Detach();
  return true;
}

bool GetBuffer(const ComPtr<IBuffer> &buffer,
               std::string *outbuffer) {
  UINT32 size;
  HRESULT hr = buffer->get_Length(&size);
  if (hr != S_OK)
    return false;

  ComPtr<Windows::Storage::Streams::IBufferByteAccess> buffer_access;
  hr = buffer.As(&buffer_access);
  if (hr != S_OK)
    return false;

  byte *data;
  hr = buffer_access->Buffer(&data);
  if (hr != S_OK)
    return false;

  outbuffer->assign(data, data + size);
  return true;
}

CryptoWinRT::CryptoWinRT(Padding padding)
    : padding_(padding) {}

CryptoWinRT::~CryptoWinRT() {
}

bool CryptoWinRT::Initialize() {
  HRESULT hr = S_OK;
  if (!engine_) {
    hr =
        GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_Core_CryptographicEngine).Get(),
                             &engine_);
    if (hr != S_OK)
      return false;
  }

  if (!provider_) {
    ComPtr<ISymmetricKeyAlgorithmProviderStatics> provide_factory;
    hr = GetActivationFactory(HString::MakeReference(
        RuntimeClass_Windows_Security_Cryptography_Core_SymmetricKeyAlgorithmProvider).Get(),
                              &provide_factory);

    if (hr != S_OK)
      return false;

    hr = provide_factory->OpenAlgorithm(HString::MakeReference(L"DES_ECB").Get(),
                                        &provider_);
    if (hr != S_OK)
      return false;
  }

  if (!buffer_factory_) {
    hr =
        Windows::Foundation::GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_Streams_Buffer).Get(),
                                                  &buffer_factory_);
  }
  return hr == S_OK;
}

bool CryptoWinRT::DoCrypto(bool encrypt,
                           const std::string &key,
                           const std::string &input,
                           std::string *output) {

  if (!Initialize())
    return false;

  Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IBuffer> key_buffer;
  if (!SetBuffer(key.data(), key.size(), buffer_factory_.Get(), &key_buffer))
    return false;

  ComPtr<ICryptographicKey> cryptographic_key;
  HRESULT hr = provider_->CreateSymmetricKey(key_buffer.Get(), &cryptographic_key);
  if (hr != S_OK)
    return false;

  Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IBuffer> input_buffer;
  if (!SetBuffer(input.data(), input.size(), buffer_factory_.Get(), &input_buffer))
    return false;

  ComPtr<IBuffer> result;
  if (encrypt) {
    hr = engine_->Encrypt(cryptographic_key.Get(),
                          input_buffer.Get(),
                          nullptr, //ECB，不需要向量
                          &result);
  } else {
    hr = engine_->Decrypt(cryptographic_key.Get(),
                          input_buffer.Get(),
                          nullptr, //ECB，不需要向量
                          &result);
  }
  if (hr != S_OK)
    return false;
  return GetBuffer(result, output);
}

bool CryptoWinRT::Encrypt(const std::string &key,
                          const std::string &plaintext,
                          std::string *ciphertext) {
  int pad = plaintext.size() % DES_BLOCK_SIZE;
  size_t buffer_size = plaintext.size();
  if (padding_ == PKCS5 || pad > 0) {
    buffer_size += (DES_BLOCK_SIZE - pad);
  }
  std::string input(plaintext);
  if (padding_ == PKCS5) {
    //需要补齐
    for (int i = plaintext.size(); i < buffer_size; ++i) {
      input += (pad ? (DES_BLOCK_SIZE - pad) : 0x08);
    }
  }
  return DoCrypto(true, key, input, ciphertext);
}

bool CryptoWinRT::Decrypt(const std::string &key,
                          const std::string &ciphertext,
                          std::string *plaintext) {

  size_t cipher_text_size = ciphertext.size();
  std::string output;
  if (!DoCrypto(false, key, ciphertext, &output))
    return false;

  uint8_t unpad = 0;
  if (padding_ == PKCS5) {
    unpad = output[cipher_text_size - 1];
    if (unpad > cipher_text_size)
      unpad = 0;
  } else {
    for (int i = cipher_text_size - 1; i > 0; i--) {
      if (output[i] > 0)
        break;
      ++unpad;
    }
  }
  plaintext->clear();
  plaintext->append(output.data(), output.data() + cipher_text_size - unpad);
  return true;
}
} //cryto