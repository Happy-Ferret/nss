/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include "nss.h"
#include "pk11pub.h"
#include "sechash.h"

#include "cpputil.h"
#include "nss_scoped_ptrs.h"

#include "gtest/gtest.h"

namespace nss_test {

// ChaCha20/Poly1305 Test Vector 1, RFC 7539
// <http://tools.ietf.org/html/rfc7539#section-2.8.2>
const uint8_t kTestVector1Data[] = {
    0x4c, 0x61, 0x64, 0x69, 0x65, 0x73, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x47,
    0x65, 0x6e, 0x74, 0x6c, 0x65, 0x6d, 0x65, 0x6e, 0x20, 0x6f, 0x66, 0x20,
    0x74, 0x68, 0x65, 0x20, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x20, 0x6f, 0x66,
    0x20, 0x27, 0x39, 0x39, 0x3a, 0x20, 0x49, 0x66, 0x20, 0x49, 0x20, 0x63,
    0x6f, 0x75, 0x6c, 0x64, 0x20, 0x6f, 0x66, 0x66, 0x65, 0x72, 0x20, 0x79,
    0x6f, 0x75, 0x20, 0x6f, 0x6e, 0x6c, 0x79, 0x20, 0x6f, 0x6e, 0x65, 0x20,
    0x74, 0x69, 0x70, 0x20, 0x66, 0x6f, 0x72, 0x20, 0x74, 0x68, 0x65, 0x20,
    0x66, 0x75, 0x74, 0x75, 0x72, 0x65, 0x2c, 0x20, 0x73, 0x75, 0x6e, 0x73,
    0x63, 0x72, 0x65, 0x65, 0x6e, 0x20, 0x77, 0x6f, 0x75, 0x6c, 0x64, 0x20,
    0x62, 0x65, 0x20, 0x69, 0x74, 0x2e};
const uint8_t kTestVector1AAD[] = {0x50, 0x51, 0x52, 0x53, 0xc0, 0xc1,
                                   0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7};
const uint8_t kTestVector1Key[] = {
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a,
    0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95,
    0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f};
const uint8_t kTestVector1IV[] = {0x07, 0x00, 0x00, 0x00, 0x40, 0x41,
                                  0x42, 0x43, 0x44, 0x45, 0x46, 0x47};
const uint8_t kTestVector1CT[] = {
    0xd3, 0x1a, 0x8d, 0x34, 0x64, 0x8e, 0x60, 0xdb, 0x7b, 0x86, 0xaf, 0xbc,
    0x53, 0xef, 0x7e, 0xc2, 0xa4, 0xad, 0xed, 0x51, 0x29, 0x6e, 0x08, 0xfe,
    0xa9, 0xe2, 0xb5, 0xa7, 0x36, 0xee, 0x62, 0xd6, 0x3d, 0xbe, 0xa4, 0x5e,
    0x8c, 0xa9, 0x67, 0x12, 0x82, 0xfa, 0xfb, 0x69, 0xda, 0x92, 0x72, 0x8b,
    0x1a, 0x71, 0xde, 0x0a, 0x9e, 0x06, 0x0b, 0x29, 0x05, 0xd6, 0xa5, 0xb6,
    0x7e, 0xcd, 0x3b, 0x36, 0x92, 0xdd, 0xbd, 0x7f, 0x2d, 0x77, 0x8b, 0x8c,
    0x98, 0x03, 0xae, 0xe3, 0x28, 0x09, 0x1b, 0x58, 0xfa, 0xb3, 0x24, 0xe4,
    0xfa, 0xd6, 0x75, 0x94, 0x55, 0x85, 0x80, 0x8b, 0x48, 0x31, 0xd7, 0xbc,
    0x3f, 0xf4, 0xde, 0xf0, 0x8e, 0x4b, 0x7a, 0x9d, 0xe5, 0x76, 0xd2, 0x65,
    0x86, 0xce, 0xc6, 0x4b, 0x61, 0x16, 0x1a, 0xe1, 0x0b, 0x59, 0x4f, 0x09,
    0xe2, 0x6a, 0x7e, 0x90, 0x2e, 0xcb, 0xd0, 0x60, 0x06, 0x91};

// ChaCha20/Poly1305 Test Vector 2, RFC 7539
// <http://tools.ietf.org/html/rfc7539#appendix-A.5>
const uint8_t kTestVector2Data[] = {
    0x49, 0x6e, 0x74, 0x65, 0x72, 0x6e, 0x65, 0x74, 0x2d, 0x44, 0x72, 0x61,
    0x66, 0x74, 0x73, 0x20, 0x61, 0x72, 0x65, 0x20, 0x64, 0x72, 0x61, 0x66,
    0x74, 0x20, 0x64, 0x6f, 0x63, 0x75, 0x6d, 0x65, 0x6e, 0x74, 0x73, 0x20,
    0x76, 0x61, 0x6c, 0x69, 0x64, 0x20, 0x66, 0x6f, 0x72, 0x20, 0x61, 0x20,
    0x6d, 0x61, 0x78, 0x69, 0x6d, 0x75, 0x6d, 0x20, 0x6f, 0x66, 0x20, 0x73,
    0x69, 0x78, 0x20, 0x6d, 0x6f, 0x6e, 0x74, 0x68, 0x73, 0x20, 0x61, 0x6e,
    0x64, 0x20, 0x6d, 0x61, 0x79, 0x20, 0x62, 0x65, 0x20, 0x75, 0x70, 0x64,
    0x61, 0x74, 0x65, 0x64, 0x2c, 0x20, 0x72, 0x65, 0x70, 0x6c, 0x61, 0x63,
    0x65, 0x64, 0x2c, 0x20, 0x6f, 0x72, 0x20, 0x6f, 0x62, 0x73, 0x6f, 0x6c,
    0x65, 0x74, 0x65, 0x64, 0x20, 0x62, 0x79, 0x20, 0x6f, 0x74, 0x68, 0x65,
    0x72, 0x20, 0x64, 0x6f, 0x63, 0x75, 0x6d, 0x65, 0x6e, 0x74, 0x73, 0x20,
    0x61, 0x74, 0x20, 0x61, 0x6e, 0x79, 0x20, 0x74, 0x69, 0x6d, 0x65, 0x2e,
    0x20, 0x49, 0x74, 0x20, 0x69, 0x73, 0x20, 0x69, 0x6e, 0x61, 0x70, 0x70,
    0x72, 0x6f, 0x70, 0x72, 0x69, 0x61, 0x74, 0x65, 0x20, 0x74, 0x6f, 0x20,
    0x75, 0x73, 0x65, 0x20, 0x49, 0x6e, 0x74, 0x65, 0x72, 0x6e, 0x65, 0x74,
    0x2d, 0x44, 0x72, 0x61, 0x66, 0x74, 0x73, 0x20, 0x61, 0x73, 0x20, 0x72,
    0x65, 0x66, 0x65, 0x72, 0x65, 0x6e, 0x63, 0x65, 0x20, 0x6d, 0x61, 0x74,
    0x65, 0x72, 0x69, 0x61, 0x6c, 0x20, 0x6f, 0x72, 0x20, 0x74, 0x6f, 0x20,
    0x63, 0x69, 0x74, 0x65, 0x20, 0x74, 0x68, 0x65, 0x6d, 0x20, 0x6f, 0x74,
    0x68, 0x65, 0x72, 0x20, 0x74, 0x68, 0x61, 0x6e, 0x20, 0x61, 0x73, 0x20,
    0x2f, 0xe2, 0x80, 0x9c, 0x77, 0x6f, 0x72, 0x6b, 0x20, 0x69, 0x6e, 0x20,
    0x70, 0x72, 0x6f, 0x67, 0x72, 0x65, 0x73, 0x73, 0x2e, 0x2f, 0xe2, 0x80,
    0x9d};
const uint8_t kTestVector2AAD[] = {0xf3, 0x33, 0x88, 0x86, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x4e, 0x91};
const uint8_t kTestVector2Key[] = {
    0x1c, 0x92, 0x40, 0xa5, 0xeb, 0x55, 0xd3, 0x8a, 0xf3, 0x33, 0x88,
    0x86, 0x04, 0xf6, 0xb5, 0xf0, 0x47, 0x39, 0x17, 0xc1, 0x40, 0x2b,
    0x80, 0x09, 0x9d, 0xca, 0x5c, 0xbc, 0x20, 0x70, 0x75, 0xc0};
const uint8_t kTestVector2IV[] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
                                  0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
const uint8_t kTestVector2CT[] = {
    0x64, 0xa0, 0x86, 0x15, 0x75, 0x86, 0x1a, 0xf4, 0x60, 0xf0, 0x62, 0xc7,
    0x9b, 0xe6, 0x43, 0xbd, 0x5e, 0x80, 0x5c, 0xfd, 0x34, 0x5c, 0xf3, 0x89,
    0xf1, 0x08, 0x67, 0x0a, 0xc7, 0x6c, 0x8c, 0xb2, 0x4c, 0x6c, 0xfc, 0x18,
    0x75, 0x5d, 0x43, 0xee, 0xa0, 0x9e, 0xe9, 0x4e, 0x38, 0x2d, 0x26, 0xb0,
    0xbd, 0xb7, 0xb7, 0x3c, 0x32, 0x1b, 0x01, 0x00, 0xd4, 0xf0, 0x3b, 0x7f,
    0x35, 0x58, 0x94, 0xcf, 0x33, 0x2f, 0x83, 0x0e, 0x71, 0x0b, 0x97, 0xce,
    0x98, 0xc8, 0xa8, 0x4a, 0xbd, 0x0b, 0x94, 0x81, 0x14, 0xad, 0x17, 0x6e,
    0x00, 0x8d, 0x33, 0xbd, 0x60, 0xf9, 0x82, 0xb1, 0xff, 0x37, 0xc8, 0x55,
    0x97, 0x97, 0xa0, 0x6e, 0xf4, 0xf0, 0xef, 0x61, 0xc1, 0x86, 0x32, 0x4e,
    0x2b, 0x35, 0x06, 0x38, 0x36, 0x06, 0x90, 0x7b, 0x6a, 0x7c, 0x02, 0xb0,
    0xf9, 0xf6, 0x15, 0x7b, 0x53, 0xc8, 0x67, 0xe4, 0xb9, 0x16, 0x6c, 0x76,
    0x7b, 0x80, 0x4d, 0x46, 0xa5, 0x9b, 0x52, 0x16, 0xcd, 0xe7, 0xa4, 0xe9,
    0x90, 0x40, 0xc5, 0xa4, 0x04, 0x33, 0x22, 0x5e, 0xe2, 0x82, 0xa1, 0xb0,
    0xa0, 0x6c, 0x52, 0x3e, 0xaf, 0x45, 0x34, 0xd7, 0xf8, 0x3f, 0xa1, 0x15,
    0x5b, 0x00, 0x47, 0x71, 0x8c, 0xbc, 0x54, 0x6a, 0x0d, 0x07, 0x2b, 0x04,
    0xb3, 0x56, 0x4e, 0xea, 0x1b, 0x42, 0x22, 0x73, 0xf5, 0x48, 0x27, 0x1a,
    0x0b, 0xb2, 0x31, 0x60, 0x53, 0xfa, 0x76, 0x99, 0x19, 0x55, 0xeb, 0xd6,
    0x31, 0x59, 0x43, 0x4e, 0xce, 0xbb, 0x4e, 0x46, 0x6d, 0xae, 0x5a, 0x10,
    0x73, 0xa6, 0x72, 0x76, 0x27, 0x09, 0x7a, 0x10, 0x49, 0xe6, 0x17, 0xd9,
    0x1d, 0x36, 0x10, 0x94, 0xfa, 0x68, 0xf0, 0xff, 0x77, 0x98, 0x71, 0x30,
    0x30, 0x5b, 0xea, 0xba, 0x2e, 0xda, 0x04, 0xdf, 0x99, 0x7b, 0x71, 0x4d,
    0x6c, 0x6f, 0x2c, 0x29, 0xa6, 0xad, 0x5c, 0xb4, 0x02, 0x2b, 0x02, 0x70,
    0x9b, 0xee, 0xad, 0x9d, 0x67, 0x89, 0x0c, 0xbb, 0x22, 0x39, 0x23, 0x36,
    0xfe, 0xa1, 0x85, 0x1f, 0x38};

class Pkcs11ChaCha20Poly1305Test : public ::testing::Test {
 public:
  void EncryptDecrypt(PK11SymKey* symKey, const uint8_t* data, size_t data_len,
                      const uint8_t* aad, size_t aad_len, const uint8_t* iv,
                      size_t iv_len, const uint8_t* ct = nullptr,
                      size_t ct_len = 0) {
    // Prepare AEAD params.
    CK_NSS_AEAD_PARAMS aead_params;
    aead_params.pNonce = toUcharPtr(iv);
    aead_params.ulNonceLen = iv_len;
    aead_params.pAAD = toUcharPtr(aad);
    aead_params.ulAADLen = aad_len;
    aead_params.ulTagLen = 16;

    SECItem params = {siBuffer, reinterpret_cast<unsigned char*>(&aead_params),
                      sizeof(aead_params)};

    // Encrypt.
    unsigned int outputLen = 0;
    std::vector<uint8_t> output(data_len + aead_params.ulTagLen);
    SECStatus rv = PK11_Encrypt(symKey, mech, &params, &output[0], &outputLen,
                                output.size(), data, data_len);
    EXPECT_EQ(rv, SECSuccess);

    // Check ciphertext and tag.
    if (ct) {
      EXPECT_TRUE(!memcmp(ct, &output[0], outputLen));
    }

    // Decrypt.
    unsigned int decryptedLen = 0;
    std::vector<uint8_t> decrypted(data_len);
    rv = PK11_Decrypt(symKey, mech, &params, &decrypted[0], &decryptedLen,
                      decrypted.size(), &output[0], outputLen);
    EXPECT_EQ(rv, SECSuccess);

    // Check the plaintext.
    EXPECT_TRUE(!memcmp(data, &decrypted[0], decryptedLen));

    // Decrypt with bogus data.
    {
      std::vector<uint8_t> bogusCiphertext(output);
      bogusCiphertext[0] ^= 0xff;
      rv = PK11_Decrypt(symKey, mech, &params, &decrypted[0], &decryptedLen,
                        decrypted.size(), &bogusCiphertext[0], outputLen);
      EXPECT_NE(rv, SECSuccess);
    }

    // Decrypt with bogus tag.
    {
      std::vector<uint8_t> bogusTag(output);
      bogusTag[outputLen - 1] ^= 0xff;
      rv = PK11_Decrypt(symKey, mech, &params, &decrypted[0], &decryptedLen,
                        decrypted.size(), &bogusTag[0], outputLen);
      EXPECT_NE(rv, SECSuccess);
    }

    // Decrypt with bogus IV.
    {
      SECItem bogusParams(params);
      CK_NSS_AEAD_PARAMS bogusAeadParams(aead_params);
      bogusParams.data = reinterpret_cast<unsigned char*>(&bogusAeadParams);

      std::vector<uint8_t> bogusIV(iv, iv + iv_len);
      bogusAeadParams.pNonce = toUcharPtr(&bogusIV[0]);
      bogusIV[0] ^= 0xff;

      rv = PK11_Decrypt(symKey, mech, &bogusParams, &decrypted[0],
                        &decryptedLen, data_len, &output[0], outputLen);
      EXPECT_NE(rv, SECSuccess);
    }

    // Decrypt with bogus additional data.
    {
      SECItem bogusParams(params);
      CK_NSS_AEAD_PARAMS bogusAeadParams(aead_params);
      bogusParams.data = reinterpret_cast<unsigned char*>(&bogusAeadParams);

      std::vector<uint8_t> bogusAAD(aad, aad + aad_len);
      bogusAeadParams.pAAD = toUcharPtr(&bogusAAD[0]);
      bogusAAD[0] ^= 0xff;

      rv = PK11_Decrypt(symKey, mech, &bogusParams, &decrypted[0],
                        &decryptedLen, data_len, &output[0], outputLen);
      EXPECT_NE(rv, SECSuccess);
    }
  }

  void EncryptDecrypt(const uint8_t* key, size_t key_len, const uint8_t* data,
                      size_t data_len, const uint8_t* aad, size_t aad_len,
                      const uint8_t* iv, size_t iv_len, const uint8_t* ct,
                      size_t ct_len) {
    ScopedPK11SlotInfo slot(PK11_GetInternalSlot());
    SECItem keyItem = {siBuffer, toUcharPtr(key),
                       static_cast<unsigned int>(key_len)};

    // Import key.
    ScopedPK11SymKey symKey(PK11_ImportSymKey(
        slot.get(), mech, PK11_OriginUnwrap, CKA_ENCRYPT, &keyItem, nullptr));
    EXPECT_TRUE(!!symKey);

    // Check.
    EncryptDecrypt(symKey.get(), data, data_len, aad, aad_len, iv, iv_len, ct,
                   ct_len);
  }

 protected:
  CK_MECHANISM_TYPE mech = CKM_NSS_CHACHA20_POLY1305;
};

#define ENCRYPT_DECRYPT(v)                                                 \
  EncryptDecrypt(v##Key, sizeof(v##Key), v##Data, sizeof(v##Data), v##AAD, \
                 sizeof(v##AAD), v##IV, sizeof(v##IV), v##CT, sizeof(v##CT));

TEST_F(Pkcs11ChaCha20Poly1305Test, GenerateEncryptDecrypt) {
  // Generate a random key.
  ScopedPK11SlotInfo slot(PK11_GetInternalSlot());
  ScopedPK11SymKey symKey(PK11_KeyGen(slot.get(), mech, nullptr, 32, nullptr));
  EXPECT_TRUE(!!symKey);

  // Generate random data.
  std::vector<uint8_t> data(512);
  SECStatus rv = PK11_GenerateRandomOnSlot(slot.get(), &data[0], data.size());
  EXPECT_EQ(rv, SECSuccess);

  // Generate random AAD.
  std::vector<uint8_t> aad(16);
  rv = PK11_GenerateRandomOnSlot(slot.get(), &aad[0], aad.size());
  EXPECT_EQ(rv, SECSuccess);

  // Generate random IV.
  std::vector<uint8_t> iv(12);
  rv = PK11_GenerateRandomOnSlot(slot.get(), &iv[0], iv.size());
  EXPECT_EQ(rv, SECSuccess);

  // Check.
  EncryptDecrypt(symKey.get(), &data[0], data.size(), &aad[0], aad.size(),
                 &iv[0], iv.size());
}

TEST_F(Pkcs11ChaCha20Poly1305Test, CheckTestVector1) {
  ENCRYPT_DECRYPT(kTestVector1);
}

TEST_F(Pkcs11ChaCha20Poly1305Test, CheckTestVector2) {
  ENCRYPT_DECRYPT(kTestVector2);
}

}  // namespace nss_test
