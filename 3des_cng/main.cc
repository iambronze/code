#include "des.h"

int main(int argc, char *argv[]) {
  crypto::Crypto3DesCNG des(crypto::PKCS5);
  des.Initialize();
  std::string plaintext = "this is a test!";
  std::string ciphertext;
  des.DoEncrypt("12345678", plaintext, &ciphertext);
  des.DoDecrypt("12345678", ciphertext, &plaintext);
  return 0;
}