#ifndef AES_H
#define AES_H
#include <string>

std::string aesEncrypt(std::string plaintext,std::string key);
std::string aesDecrypt(std::string ciphertext, std::string key);

#endif