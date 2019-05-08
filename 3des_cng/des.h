#ifndef DES_H_
#define DES_H_

#include <string>
#include <windows.h>
#include <bcrypt.h>

namespace crypto {
enum Padding {
  NOPKCS,
  PKCS5
};

class Crypto3DesCNG {
public:
  explicit Crypto3DesCNG(Padding padding);

  virtual ~Crypto3DesCNG();

  bool Initialize();

  bool DoEncrypt(const std::string &key,
                 const std::string &plaintext,
                 std::string *ciphertext);

  bool DoDecrypt(const std::string &key,
                 const std::string &ciphertext,
                 std::string *plaintext);
private:
  void Destroy();

  BCRYPT_KEY_HANDLE CreateKeyHandle(const std::string &key);

  Padding padding_;
  uint32_t block_size_;
  BCRYPT_ALG_HANDLE alg_handle_;
};
} //crypto

#endif  //DES_H_