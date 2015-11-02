#include "openssl/err.h"
#include "openssl/evp.h"
#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/rsa.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <array>
#include <algorithm>
#include <iostream>
#include <random>

//
// g++ -g -Wno-deprecated --std=c++14 crypto.cpp $(pkg-config --cflags --libs openssl)

template <typename Type>
class guard_ptr {
 public:
  guard_ptr(Type* ptr, std::function<void(Type*)> fn)
    : pointer_{ptr}, onexit_{fn} {}

  ~guard_ptr() {
    if (pointer_) {
      onexit_(pointer_);
    }
  }

  explicit operator bool() const {
    return pointer_ != nullptr;
  }

  Type& operator*() const { return *pointer_; }
  Type* operator->() const { return pointer_; }
  Type* get() const { return pointer_; }

  Type* release() {
    auto ptr = pointer_;
    pointer_ = nullptr;
    return ptr;
  }

 private:
  Type* pointer_ = nullptr;
  std::function<void(Type*)> onexit_;
};

namespace {

const char b64_table[65] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789+/";

}  // namespace

std::string base64_encode(const std::string& input)
{
  std::string result;
  result.reserve(4*((input.size()+2)/3));

  auto it = std::begin(input);
  auto end = std::end(input);
  size_t pad;

  while (it != end) {
    uint32_t accum = *it++ << 16; pad = 0;
    if (it != end) accum |= *it++ << 8; else ++pad;
    if (it != end) accum |= *it++ << 0; else ++pad;

    result += b64_table[accum >> 18 & 0x3f];
    result += b64_table[accum >> 12 & 0x3f];
    result += b64_table[accum >> 6  & 0x3f];
    result += b64_table[accum >> 0  & 0x3f];
  }

  if (pad >= 1) result[result.size()-1] = '=';
  if (pad >= 2) result[result.size()-2] = '=';

  return result;
}

template <typename Type>
struct Buffer {
  Type  *base = nullptr;
  size_t size = 0;

  Buffer(Type *base, size_t size) :
    base {base}, size {size} {}

  template <typename T>
    Buffer(const Buffer<T> &copy) :
      base {copy.base}, size {copy.size} {}

  template <typename T>
    Buffer &operator=(const Buffer<T> &that) {
      base = that.base;
      size = that.size;
      return *this;
    }

  explicit operator bool() const {
    return base;
  }
};

using ByteBuffer = Buffer<uint8_t>;
using ConstByteBuffer = Buffer<const uint8_t>;

template <typename Type>
Type *begin(const Buffer<Type> &buffer) { return buffer.base; }

template <typename Type>
Type *end(const Buffer<Type> &buffer) { return buffer.base + buffer.size; }

template <typename Type>
Buffer<Type> grow(const Buffer<Type> &buffer, int size) {
  return { buffer.base, buffer.size + size };
}

template <typename Type>
Buffer<Type> slide(const Buffer<Type> &buffer, int size) {
  return { buffer.base + size, buffer.size };
}

template <typename Type>
Buffer<Type> advance(const Buffer<Type> &buffer, int size) {
  return { buffer.base + size, buffer.size - size };
}

template <typename Type>
Buffer<Type> align_size(const Buffer<Type> &buffer, int alignment) {
  return { buffer.base, buffer.size - (buffer.size % alignment) };
}

template <typename SrcType, typename DstType>
size_t copy(const Buffer<SrcType> &source, const Buffer<DstType> &destination) {
  size_t count = std::min(source.size, destination.size);
  std::copy_n(source.base, count, destination.base);
  return count;
}

template <typename SrcType, typename DstType>
size_t copy(const Buffer<SrcType> &source, const Buffer<DstType> &destination, size_t count) {
  count = std::min({count, destination.size, source.size});
  std::copy_n(source.base, count, destination.base);
  return count;
}

template <typename Type>
std::ostream &operator<<(std::ostream &out, const Buffer<Type> buffer) {
  for (const auto& item : buffer)
    out << item;
  return out;
}

template <size_t Bits>
struct Block {
  Block() {
    std::random_device device;
    std::uniform_int_distribution<uint8_t> distribution;
    std::generate(begin(bytes), end(bytes), [&device, &distribution] {
          return distribution(device);
        });
  }

  Block(const uint8_t* buf, size_t size) {
    assert(size >= bytes.size() && "Insufficient input");
    memcpy(bytes.data(), buf, size);
  }

  ~Block() {
    bytes.fill(0);  // Zero out memory after use
  }

  std::array<uint8_t, Bits/8> bytes;
};

struct Password {
  enum class Version : uint8_t { none, v1 };
  enum class Algorithm : uint8_t { none, AES256 };
  enum class Mode : uint8_t { none, CBC_PKCS1_OAEP };

  std::vector<uint8_t> serialize() const {
    const size_t size =
      sizeof(Version) + sizeof(Algorithm) + sizeof(Mode) + key.bytes.size() + iv.bytes.size();
    std::vector<uint8_t> output(size);

    output[0] = static_cast<char>(version);
    output[1] = static_cast<char>(algorithm);
    output[2] = static_cast<char>(mode);

    auto iter = &output[3];
    iter = std::copy_n(begin(key.bytes), key.bytes.size(), iter);
    iter = std::copy_n(begin(iv.bytes), iv.bytes.size(), iter);

    return output;
  }

  void deserialize(const std::vector<uint8_t>& input) {
    version   = static_cast<Version>(input[0]);
    algorithm = static_cast<Algorithm>(input[1]);
    mode      = static_cast<Mode>(input[2]);

    auto keybegin = &input[3];
    auto ivbegin = keybegin + key.bytes.size();
    std::copy_n(keybegin, key.bytes.size(), begin(key.bytes));
    std::copy_n(ivbegin, iv.bytes.size(), begin(iv.bytes));
  }

  std::string to_string() const {
    auto buffer = serialize();
    return base64_encode({
      reinterpret_cast<const char*>(buffer.data()),
        buffer.size()
    });
  }

  Version version = Version::v1;
  Algorithm algorithm = Algorithm::AES256;
  Mode mode = Mode::CBC_PKCS1_OAEP;

  Block<256> key;
  Block<128> iv;
};

bool operator==(const Password& a, const Password &b) {
  return
    a.version == b.version &&
    a.algorithm == b.algorithm &&
    a.mode == b.mode &&
    a.key.bytes == b.key.bytes &&
    a.iv.bytes == b.iv.bytes;
}

std::string openssl_last_error() {
  char buf[256];  // tmp buf (width taken from impl)
  return ERR_error_string(ERR_get_error(), buf);
}

struct PemCertificate {
  std::string text;
};


class PublicKey {
 public:
  class Encryptor {
   public:
    explicit Encryptor(EVP_PKEY* key)
        : context_{EVP_CIPHER_CTX_new()} {
      if (context_) {
        auto cipher = EVP_aes_256_cbc();
#if true
        assert(EVP_CIPHER_key_length(cipher) == password_.key.bytes.size());
        assert(EVP_CIPHER_iv_length(cipher) == password_.iv.bytes.size());

        error_ = EVP_EncryptInit(context_, cipher,
            password_.key.bytes.data(), password_.iv.bytes.data()) != 1;
#else
        uint8_t keymem[1024], ivmem[EVP_MAX_IV_LENGTH];
        uint8_t *keybuf = keymem, *ivbuf = ivmem;
        int ivlen = EVP_CIPHER_iv_length(cipher);
        int keylen = EVP_PKEY_size(key);

        assert(keylen <= sizeof(keymem) && "Key buffer overrun");
        assert(ivlen <= sizeof(ivmem) && "IV buffer overrun");
        // Generate key for provided cipher, and encrypt with certificate
        error_ = EVP_SealInit(context_, cipher, &keybuf, &keylen, ivbuf, &key, 1) != 1;
        if (!error_) {
          assert(keylen >= password_.key.bytes.size() && "Key buffer underrun");
          assert(ivlen >= password_.iv.bytes.size() && "IV buffer underrun");

          memcpy(password_.key.bytes.data(), keybuf, password_.key.bytes.size());
          memcpy(password_.iv.bytes.data(), ivbuf, password_.iv.bytes.size());
        }
#endif
      }
    }

    ~Encryptor() {
      EVP_CIPHER_CTX_free(context_);
    }

    explicit operator bool() const {
      return context_ != nullptr && !error_;
    }

    Password password() const {
      return password_;
    }

    size_t write(ConstByteBuffer input, ByteBuffer output) {
      const int block =
        std::max(std::min<int>(EVP_CIPHER_CTX_block_size(context_), input.size), 1);

      input = align_size(input, block);
      output = align_size(output, block);

      int requested = std::min(input.size, output.size);
      int written = 0;

      error_ = error_
        || EVP_EncryptUpdate(context_, output.base, &written, input.base, requested) != 1;

      return !error_ ? written : 0;
    }

    size_t finish(ByteBuffer output) {
      int written = 0;
      error_ = EVP_EncryptFinal(context_, output.base, &written) != 1;
      return !error_ ? written : 0;
    }

   private:
    EVP_CIPHER_CTX* context_ = nullptr;
    Password password_;
    bool error_ = false;
  };

  PublicKey (const PemCertificate& cert) { // calls here are null-safe
    guard_ptr<BIO> bio {
      BIO_new_mem_buf(const_cast<char*>(cert.text.data()),  // actually read-only
          cert.text.size()),
      [](auto buf) {
        BIO_free(buf);
      }
    };

    guard_ptr<X509> certificate {
      PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr),
      [](auto cert) {
        X509_free(cert);
      }
    };

    if ((key_ = X509_get_pubkey(certificate.get())) == nullptr) {
      std::cout << "Error found: " << openssl_last_error() << std::endl;
    }
  }

  ~PublicKey() {
    EVP_PKEY_free(key_);
  }

  explicit operator bool() const {
    return key_ != nullptr;
  }

  Encryptor encryptor() const {
    return Encryptor {key_};
  }

  size_t encrypt(const Password& password, ByteBuffer output) {
    std::vector<uint8_t> buffer = password.serialize();
    const int size =
      std::max(std::min<int>(buffer.size(), output.size), 0);

    // TODO: ensure RNG is seeded
    int written = RSA_public_encrypt(size, buffer.data(), output.base,
        EVP_PKEY_get1_RSA(key_), RSA_PKCS1_OAEP_PADDING);

    return written > buffer.size() ? written : 0;
  }

 private:
  EVP_PKEY* key_ = nullptr;
};

struct PemPrivateKey {
  std::string text;
};

class PrivateKey {
 public:
  class Decryptor {
   public:
    Decryptor(EVP_PKEY* key, Password password)
        : context_{EVP_CIPHER_CTX_new()},
          password_(password) {
      if (context_) {
        auto cipher = EVP_aes_256_cbc();
#if true
        assert(EVP_CIPHER_key_length(cipher) == password_.key.bytes.size());
        assert(EVP_CIPHER_iv_length(cipher) == password_.iv.bytes.size());

        error_ = EVP_DecryptInit(context_, cipher,
            password_.key.bytes.data(), password_.iv.bytes.data()) != 1;
#else
        error_ = EVP_OpenInit(context_, cipher,
            password_.key.bytes.data(), password_.key.bytes.size(),
            password_.iv.bytes.data(), key) != 1;
#endif
      }
    }

    ~Decryptor() {
      EVP_CIPHER_CTX_free(context_);
    }

    explicit operator bool() const {
      return context_ != nullptr && !error_;
    }

    size_t write(ConstByteBuffer input, ByteBuffer output) {
      const int block =
        std::max(std::min<int>(EVP_CIPHER_CTX_block_size(context_), input.size), 1);

      input = align_size(input, block);
      output = align_size(output, block);

      int requested = std::min(input.size, output.size);
      int written = 0;

      error_ = error_
        || EVP_DecryptUpdate(context_, output.base, &written, input.base, requested) != 1;

      return !error_ ? written : 0;
    }

    size_t finish(ByteBuffer output) {
      int written = 0;
      error_ = EVP_DecryptFinal(context_, output.base, &written) != 1;
      return !error_ ? written : 0;
    }

   private:
    EVP_CIPHER_CTX* context_ = nullptr;
    Password password_;
    bool error_ = false;
  };

  PrivateKey (const PemPrivateKey& key) {  // calls here are null-safe
    guard_ptr<BIO> bio {
      BIO_new_mem_buf(const_cast<char*>(key.text.data()),  // actually read-only
          key.text.size()),
      [](auto buf) {
        BIO_free(buf);
      }
    };

    if ((key_ = PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr)) == nullptr) {
      std::cout << "Error found: " << openssl_last_error() << std::endl;
    }
  }

  ~PrivateKey() {
    EVP_PKEY_free(key_);
  }

  explicit operator bool() const {
    return key_ != nullptr;
  }

  Decryptor decryptor(Password password) const {
    return Decryptor {key_, password};
  }

  size_t decrypt(ConstByteBuffer input, Password& password) {  // NOLINT
    std::vector<uint8_t> buffer = password.serialize();
    const int size =
      std::max(std::min<int>(EVP_PKEY_size(key_), input.size), 0);

    int written = RSA_private_decrypt(size, input.base, buffer.data(),
        EVP_PKEY_get1_RSA(key_), RSA_PKCS1_OAEP_PADDING);

    password.deserialize(buffer);

    return written == buffer.size() ? written : 0;
  }

 private:
  EVP_PKEY* key_ = nullptr;
};


int main() {
  ERR_load_crypto_strings();
  OpenSSL_add_all_algorithms();

  PemCertificate pemcert {
    "-----BEGIN CERTIFICATE-----\n"
    "MIICsDCCAhmgAwIBAgIJAPwg+9CydIqWMA0GCSqGSIb3DQEBBQUAMEUxCzAJBgNV\n"
    "BAYTAkFVMRMwEQYDVQQIEwpTb21lLVN0YXRlMSEwHwYDVQQKExhJbnRlcm5ldCBX\n"
    "aWRnaXRzIFB0eSBMdGQwHhcNMTUxMTA1MjM1OTQwWhcNMTYxMTA0MjM1OTQwWjBF\n"
    "MQswCQYDVQQGEwJBVTETMBEGA1UECBMKU29tZS1TdGF0ZTEhMB8GA1UEChMYSW50\n"
    "ZXJuZXQgV2lkZ2l0cyBQdHkgTHRkMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKB\n"
    "gQDtfcSEezim/R08O2O7TiwiATOrNmocf1CqlVbgBexGooVS5Gvj9c03NfI0HcHn\n"
    "hAVkvTuHbja3k68p8Z4a5CUOJ/faV8OKR5fKGohNxPGR1i5SW1avGOkJ77s8k7fg\n"
    "cgd/+xAtDXtdmuB7KKORop+IrkIPfKUqpzTec9JZ29iuKQIDAQABo4GnMIGkMB0G\n"
    "A1UdDgQWBBSh0XB21iGiE4ccqXEZTBDKcan6kTB1BgNVHSMEbjBsgBSh0XB21iGi\n"
    "E4ccqXEZTBDKcan6kaFJpEcwRTELMAkGA1UEBhMCQVUxEzARBgNVBAgTClNvbWUt\n"
    "U3RhdGUxITAfBgNVBAoTGEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZIIJAPwg+9Cy\n"
    "dIqWMAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcNAQEFBQADgYEAetzmwkVVdoCUusus\n"
    "hCjSvJbhLGUeVW/JEREGpT3SP9jI71Y3rE7CWGhxQjWeKOcl8Tl3+pbNIERPE8mJ\n"
    "VlsDl5IXO5Exbv7Yrk8+9yW6HgXOl7ubw8UCQWlV/o5+lT0swT6eYfL7kC0fjKbp\n"
    "9SXlCIaw/jxPI5CBSuhfSI902dw=\n"
    "-----END CERTIFICATE-----\n"
  };

  PemPrivateKey pemkey {
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIICWwIBAAKBgQDtfcSEezim/R08O2O7TiwiATOrNmocf1CqlVbgBexGooVS5Gvj\n"
    "9c03NfI0HcHnhAVkvTuHbja3k68p8Z4a5CUOJ/faV8OKR5fKGohNxPGR1i5SW1av\n"
    "GOkJ77s8k7fgcgd/+xAtDXtdmuB7KKORop+IrkIPfKUqpzTec9JZ29iuKQIDAQAB\n"
    "AoGALBKE35kGYGbkgAL9dQsCwaS7c/B7LKKr05w3LQesW0dZupJNO1aoKLDIK8fB\n"
    "7jbf0VwUqgNYACVWAlCmaJMiFOzdYVinJlPLiMpEHP107RTGdHO499eXq98yT1Ke\n"
    "aj+ey9S6HaAhQ9l1RZgYGYecIN4uccQqpG+Rf3HtEYxkDSECQQD/8Vcvpd++aVn4\n"
    "2YUjnGqpRXiYTmOYGAklj+K3AXwiJ9L6GXiKkSBL8ZBddNU98sd4G5ygaPkCXTc+\n"
    "i1AWclSNAkEA7YteyHVAjfEBFJI279kJKmS4X8bXV5H+PtyzTTRqf+2UbamoYcPN\n"
    "29rbMERh41BWvQFyAWY8y6iQ3bO/RLavDQJAKvle5kU3uEUAMmRzknMlBZ8AjLI8\n"
    "zsDaaFAshQXeze1Z41x7fOi5P4cj8k03sNse2u/n8JcvmFIGgJ3rqJkx0QJAWLgN\n"
    "7XaFZr4VdjZp2EjKOJAnoHXmZal8OMQ7H2GtSRxVrGOKJQF5eFyUMsHHgZu22Z6Z\n"
    "ktY5bKMHgBYrIKHOXQJAb2xkeEdPkuomrh7vIMwrMZCu8E/nId7CUtXK50vsHKsA\n"
    "k7P3eT67pczdKbG2ABfBc1YtAFmCrAGyALUWQbF6HQ==\n"
    "-----END RSA PRIVATE KEY-----\n"
  };


  PublicKey publickey{pemcert};
  PrivateKey privatekey{pemkey};

  if (publickey && privatekey) {
    uint8_t cipherbuf[1024], plainbuf[1024];
    memset(cipherbuf, 0, sizeof(cipherbuf));
    memcpy(plainbuf, pemcert.text.data(), pemcert.text.size());

    ByteBuffer available {cipherbuf, sizeof(cipherbuf)};
    ByteBuffer ciphertext {cipherbuf, 0};
    ByteBuffer plaintext {plainbuf, pemcert.text.size()};

    auto encryptor = publickey.encryptor();
    if (!encryptor) {
      std::cout << "failed to create encryptor: " << openssl_last_error() << std::endl;
      return -1;
    }

    while (encryptor) {
      int written = encryptor.write(plaintext, available);
      available  = advance(available, written);
      plaintext  = advance(plaintext, written);
      ciphertext = grow(ciphertext, written);
      if (!written) break;
    }

    int written = encryptor.finish(available);
    ciphertext = grow(ciphertext, written);

    memset(plainbuf, 0, sizeof(plainbuf));
    available = {plainbuf, sizeof(plainbuf)};
    plaintext = {plainbuf, 0};

    auto decryptor = privatekey.decryptor(encryptor.password());
    if (!decryptor) {
      std::cout << "failed to create decryptor: " << openssl_last_error() << std::endl;
      return -1;
    }

    while (decryptor) {
      int written = decryptor.write(ciphertext, available);
      available  = advance(available, written);
      ciphertext = advance(ciphertext, written);
      plaintext  = grow(plaintext, written);
      if (!written) break;
    }

    written = decryptor.finish(available);
    plaintext = grow(plaintext, written);

    std::cout << "original: " << pemcert.text << std::endl;
    std::cout << "output: " << plaintext << std::endl;
  } else {
    std::cout << "failed to read public or private key: " << openssl_last_error() << std::endl;
  }
}
