#ifndef DES_H_
#define DES_H_

#include <string>
#include <windows.security.cryptography.h>
#include <windows.security.cryptography.core.h>
#include <windows.security.cryptography.certificates.h>
#include <Windows.Foundation.h>
#include <wrl.h>
#include <windows.storage.streams.h>
#include <wrl/event.h>
#include <stdio.h>
#include <Objbase.h>
#include <robuffer.h>

using namespace ABI::Windows::Foundation;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Security::Cryptography;
using namespace ABI::Windows::Security::Cryptography::Certificates;
using namespace ABI::Windows::Security::Cryptography::Core;
using namespace ABI::Windows::Storage::Streams;

namespace crypto {
enum Padding {
  NOPKCS,
  PKCS5
};

class CryptoWinRT {
public:
  explicit CryptoWinRT(Padding padding);

  virtual ~CryptoWinRT();

  bool Encrypt(const std::string &key,
               const std::string &plaintext,
               std::string *ciphertext);

  bool Decrypt(const std::string &key,
               const std::string &ciphertext,
               std::string *plaintext);
private:
  bool Initialize();

  bool DoCrypto(bool encrypt,
                const std::string &key,
                const std::string &input,
                std::string *output);

  ComPtr<ICryptographicEngineStatics> engine_;

  ComPtr<ISymmetricKeyAlgorithmProvider> provider_;

  Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IBufferFactory> buffer_factory_;

  Padding padding_;
};
} //crypto

#endif  //CRYPTO_DES_H_