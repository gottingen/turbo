// Copyright 2023 The turbo Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "turbo/network/util/ssl_box.h"
#include "turbo/network/util/once_token.h"
#include "turbo/network/util/ssl_util.h"

#if defined(ENABLE_OPENSSL)
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/conf.h>
#include <openssl/bio.h>
#include <openssl/ossl_typ.h>
#endif //defined(ENABLE_OPENSSL)

#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
//openssl版本是否支持sni
#define SSL_ENABLE_SNI
#endif

using namespace std;

namespace turbo {

static bool s_ignore_invalid_cer = true;

SSL_Initor &SSL_Initor::Instance() {
    static SSL_Initor obj;
    return obj;
}

void SSL_Initor::ignoreInvalidCertificate(bool ignore) {
    s_ignore_invalid_cer = ignore;
}

SSL_Initor::SSL_Initor() {
#if defined(ENABLE_OPENSSL)
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_digests();
    OpenSSL_add_all_ciphers();
    OpenSSL_add_all_algorithms();
    CRYPTO_set_locking_callback([](int mode, int n, const char *file, int line) {
        static mutex *s_mutexes = new mutex[CRYPTO_num_locks()];
        static onceToken token(nullptr, []() {
            delete[] s_mutexes;
        });
        if (mode & CRYPTO_LOCK) {
            s_mutexes[n].lock();
        } else {
            s_mutexes[n].unlock();
        }
    });

    CRYPTO_set_id_callback([]() -> unsigned long {
#if !defined(_WIN32)
        return (unsigned long) pthread_self();
#else
        return (unsigned long) GetCurrentThreadId();
#endif
    });

    setContext("", SSLUtil::makeSSLContext(vector<shared_ptr<X509> >(), nullptr, false), false);
    setContext("", SSLUtil::makeSSLContext(vector<shared_ptr<X509> >(), nullptr, true), true);
#endif //defined(ENABLE_OPENSSL)
}

SSL_Initor::~SSL_Initor() {
#if defined(ENABLE_OPENSSL)
    EVP_cleanup();
    ERR_free_strings();
    ERR_clear_error();
#if OPENSSL_VERSION_NUMBER >= 0x10000000L && OPENSSL_VERSION_NUMBER < 0x10100000L
    ERR_remove_thread_state(nullptr);
#elif OPENSSL_VERSION_NUMBER < 0x10000000L
    ERR_remove_state(0);
#endif
    CRYPTO_set_locking_callback(nullptr);
    //sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
    CRYPTO_cleanup_all_ex_data();
    CONF_modules_unload(1);
    CONF_modules_free();
#endif //defined(ENABLE_OPENSSL)
}

bool SSL_Initor::loadCertificate(const string &pem_or_p12, bool server_mode, const string &password, bool is_file,
                                 bool is_default) {
    auto cers = SSLUtil::loadPublicKey(pem_or_p12, password, is_file);
    auto key = SSLUtil::loadPrivateKey(pem_or_p12, password, is_file);
    auto ssl_ctx = SSLUtil::makeSSLContext(cers, key, server_mode, true);
    if (!ssl_ctx) {
        return false;
    }
    for (auto &cer : cers) {
        auto server_name = SSLUtil::getServerName(cer.get());
        setContext(server_name, ssl_ctx, server_mode, is_default);
        break;
    }
    return true;
}

int SSL_Initor::findCertificate(SSL *ssl, int *, void *arg) {
#if !defined(ENABLE_OPENSSL) || !defined(SSL_ENABLE_SNI)
    return 0;
#else
    if (!ssl) {
        return SSL_TLSEXT_ERR_ALERT_FATAL;
    }

    SSL_CTX *ctx = nullptr;
    static auto &ref = SSL_Initor::Instance();
    const char *vhost = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);

    if (vhost && vhost[0] != '\0') {
        //根据域名找到证书
        ctx = ref.getSSLCtx(vhost, (bool) (arg)).get();
        if (!ctx) {
            //未找到对应的证书
            WarnL << "Can not find any certificate of host: " << vhost
                  << ", select default certificate of: " << ref._default_vhost[(bool) (arg)];
        }
    }

    if (!ctx) {
        //客户端未指定域名或者指定的证书不存在，那么选择一个默认的证书
        ctx = ref.getSSLCtx("", (bool) (arg)).get();
    }

    if (!ctx) {
        //未有任何有效的证书
        WarnL << "Can not find any available certificate of host: " << (vhost ? vhost : "default host")
              << ", tls handshake failed";
        return SSL_TLSEXT_ERR_ALERT_FATAL;
    }

    SSL_set_SSL_CTX(ssl, ctx);
    return SSL_TLSEXT_ERR_OK;
#endif
}

bool SSL_Initor::setContext(const string &vhost, const shared_ptr<SSL_CTX> &ctx, bool server_mode, bool is_default) {
    if (!ctx) {
        return false;
    }
    setupCtx(ctx.get());
#if defined(ENABLE_OPENSSL)
    if (vhost.empty()) {
        _ctx_empty[server_mode] = ctx;
#ifdef SSL_ENABLE_SNI
        if (server_mode) {
            SSL_CTX_set_tlsext_servername_callback(ctx.get(), findCertificate);
            SSL_CTX_set_tlsext_servername_arg(ctx.get(), (void *) server_mode);
        }
#endif // SSL_ENABLE_SNI

    } else {
        _ctxs[server_mode][vhost] = ctx;
        if (is_default) {
            _default_vhost[server_mode] = vhost;
        }
        if (vhost.find("*.") == 0) {
            //通配符证书
            _ctxs_wildcards[server_mode][vhost.substr(1)] = ctx;
        }
        DebugL << "Add certificate of: " << vhost;
    }
    return true;
#else
    WarnL << "ENABLE_OPENSSL disabled, you can not use any features based on openssl";
    return false;
#endif //defined(ENABLE_OPENSSL)
}

void SSL_Initor::setupCtx(SSL_CTX *ctx) {
#if defined(ENABLE_OPENSSL)
    //加载默认信任证书
    SSLUtil::loadDefaultCAs(ctx);
    SSL_CTX_set_cipher_list(ctx, "ALL:!ADH:!LOW:!EXP:!MD5:!3DES:@STRENGTH");
    SSL_CTX_set_verify_depth(ctx, 9);
    SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
    SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, [](int ok, X509_STORE_CTX *pStore) {
        if (!ok) {
            int depth = X509_STORE_CTX_get_error_depth(pStore);
            int err = X509_STORE_CTX_get_error(pStore);
            WarnL << "SSL_CTX_set_verify callback, depth: " << depth << " ,err: " << X509_verify_cert_error_string(err);
        }
        return s_ignore_invalid_cer ? 1 : ok;
    });

#ifndef SSL_OP_NO_COMPRESSION
#define SSL_OP_NO_COMPRESSION 0
#endif
#ifndef SSL_MODE_RELEASE_BUFFERS    /* OpenSSL >= 1.0.0 */
#define SSL_MODE_RELEASE_BUFFERS 0
#endif
    unsigned long ssloptions = SSL_OP_ALL
                               | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION
                               | SSL_OP_NO_COMPRESSION;

#ifdef SSL_OP_NO_RENEGOTIATION /* openssl 1.1.0 */
    ssloptions |= SSL_OP_NO_RENEGOTIATION;
#endif
    SSL_CTX_set_options(ctx, ssloptions);

#endif //defined(ENABLE_OPENSSL)
}

shared_ptr<SSL> SSL_Initor::makeSSL(bool server_mode) {
#if defined(ENABLE_OPENSSL)
#ifdef SSL_ENABLE_SNI
    //openssl 版本支持SNI
    return SSLUtil::makeSSL(_ctx_empty[server_mode].get());
#else
    //openssl 版本不支持SNI，选择默认证书
    return SSLUtil::makeSSL(getSSLCtx("",server_mode).get());
#endif//SSL_CTRL_SET_TLSEXT_HOSTNAME
#else
    return nullptr;
#endif //defined(ENABLE_OPENSSL)
}

bool SSL_Initor::trustCertificate(X509 *cer, bool server_mode) {
    return SSLUtil::trustCertificate(_ctx_empty[server_mode].get(), cer);
}

bool SSL_Initor::trustCertificate(const string &pem_p12_cer, bool server_mode, const string &password, bool is_file) {
    auto cers = SSLUtil::loadPublicKey(pem_p12_cer, password, is_file);
    for (auto &cer : cers) {
        trustCertificate(cer.get(), server_mode);
    }
    return true;
}

std::shared_ptr<SSL_CTX> SSL_Initor::getSSLCtx(const string &vhost, bool server_mode) {
    auto ret = getSSLCtx_l(vhost, server_mode);
    if (ret) {
        return ret;
    }
    return getSSLCtxWildcards(vhost, server_mode);
}

std::shared_ptr<SSL_CTX> SSL_Initor::getSSLCtxWildcards(const string &vhost, bool server_mode) {
    for (auto &pr : _ctxs_wildcards[server_mode]) {
        auto pos = strcasestr(vhost.data(), pr.first.data());
        if (pos && pos + pr.first.size() == &vhost.back() + 1) {
            return pr.second;
        }
    }
    return nullptr;
}

std::shared_ptr<SSL_CTX> SSL_Initor::getSSLCtx_l(const string &vhost_in, bool server_mode) {
    auto vhost = vhost_in;
    if (vhost.empty()) {
        if (!_default_vhost[server_mode].empty()) {
            vhost = _default_vhost[server_mode];
        } else {
            //没默认主机，选择空主机
            if (server_mode) {
                WarnL << "Server with ssl must have certification and key";
            }
            return _ctx_empty[server_mode];
        }
    }
    //根据主机名查找证书
    auto it = _ctxs[server_mode].find(vhost);
    if (it == _ctxs[server_mode].end()) {
        return nullptr;
    }
    return it->second;
}

string SSL_Initor::defaultVhost(bool server_mode) {
    return _default_vhost[server_mode];
}

////////////////////////////////////////////////////SSL_Box////////////////////////////////////////////////////////////

SSL_Box::~SSL_Box() {}

SSL_Box::SSL_Box(bool server_mode, bool enable, int buff_size) {
#if defined(ENABLE_OPENSSL)
    _read_bio = BIO_new(BIO_s_mem());
    _server_mode = server_mode;
    if (enable) {
        _ssl = SSL_Initor::Instance().makeSSL(server_mode);
    }
    if (_ssl) {
        _write_bio = BIO_new(BIO_s_mem());
        SSL_set_bio(_ssl.get(), _read_bio, _write_bio);
        _server_mode ? SSL_set_accept_state(_ssl.get()) : SSL_set_connect_state(_ssl.get());
    } else {
        WarnL << "makeSSL failed";
    }
    _send_handshake = false;
    _buff_size = buff_size;
#endif //defined(ENABLE_OPENSSL)
}

void SSL_Box::shutdown() {
#if defined(ENABLE_OPENSSL)
    _buffer_send.clear();
    int ret = SSL_shutdown(_ssl.get());
    if (ret != 1) {
        ErrorL << "SSL_shutdown failed: " << SSLUtil::getLastError();
    } else {
        flush();
    }
#endif //defined(ENABLE_OPENSSL)
}

void SSL_Box::onRecv(const Buffer::Ptr &buffer) {
    if (!buffer->size()) {
        return;
    }
    if (!_ssl) {
        if (_on_dec) {
            _on_dec(buffer);
        }
        return;
    }
#if defined(ENABLE_OPENSSL)
    uint32_t offset = 0;
    while (offset < buffer->size()) {
        auto nwrite = BIO_write(_read_bio, buffer->data() + offset, buffer->size() - offset);
        if (nwrite > 0) {
            //部分或全部写入bio完毕
            offset += nwrite;
            flush();
            continue;
        }
        //nwrite <= 0,出现异常
        ErrorL << "Ssl error on BIO_write: " << SSLUtil::getLastError();
        shutdown();
        break;
    }
#endif //defined(ENABLE_OPENSSL)
}

void SSL_Box::onSend(Buffer::Ptr buffer) {
    if (!buffer->size()) {
        return;
    }
    if (!_ssl) {
        if (_on_enc) {
            _on_enc(buffer);
        }
        return;
    }
#if defined(ENABLE_OPENSSL)
    if (!_server_mode && !_send_handshake) {
        _send_handshake = true;
        SSL_do_handshake(_ssl.get());
    }
    _buffer_send.emplace_back(std::move(buffer));
    flush();
#endif //defined(ENABLE_OPENSSL)
}

void SSL_Box::setOnDecData(const function<void(const Buffer::Ptr &)> &cb) {
    _on_dec = cb;
}

void SSL_Box::setOnEncData(const function<void(const Buffer::Ptr &)> &cb) {
    _on_enc = cb;
}

void SSL_Box::flushWriteBio() {
#if defined(ENABLE_OPENSSL)
    int total = 0;
    int nread = 0;
    auto buffer_bio = _buffer_pool.obtain2();
    buffer_bio->setCapacity(_buff_size);
    auto buf_size = buffer_bio->getCapacity() - 1;
    do {
        nread = BIO_read(_write_bio, buffer_bio->data() + total, buf_size - total);
        if (nread > 0) {
            total += nread;
        }
    } while (nread > 0 && buf_size - total > 0);

    if (!total) {
        //未有数据
        return;
    }

    //触发此次回调
    buffer_bio->data()[total] = '\0';
    buffer_bio->setSize(total);
    if (_on_enc) {
        _on_enc(buffer_bio);
    }

    if (nread > 0) {
        //还有剩余数据，读取剩余数据
        flushWriteBio();
    }
#endif //defined(ENABLE_OPENSSL)
}

void SSL_Box::flushReadBio() {
#if defined(ENABLE_OPENSSL)
    int total = 0;
    int nread = 0;
    auto buffer_bio = _buffer_pool.obtain2();
    buffer_bio->setCapacity(_buff_size);
    auto buf_size = buffer_bio->getCapacity() - 1;
    do {
        nread = SSL_read(_ssl.get(), buffer_bio->data() + total, buf_size - total);
        if (nread > 0) {
            total += nread;
        }
    } while (nread > 0 && buf_size - total > 0);

    if (!total) {
        //未有数据
        return;
    }

    //触发此次回调
    buffer_bio->data()[total] = '\0';
    buffer_bio->setSize(total);
    if (_on_dec) {
        _on_dec(buffer_bio);
    }

    if (nread > 0) {
        //还有剩余数据，读取剩余数据
        flushReadBio();
    }
#endif //defined(ENABLE_OPENSSL)
}

void SSL_Box::flush() {
#if defined(ENABLE_OPENSSL)
    if (_is_flush) {
        return;
    }
    onceToken token([&] {
        _is_flush = true;
    }, [&]() {
        _is_flush = false;
    });

    flushReadBio();
    if (!SSL_is_init_finished(_ssl.get()) || _buffer_send.empty()) {
        //ssl未握手结束或没有需要发送的数据
        flushWriteBio();
        return;
    }

    //加密数据并发送
    while (!_buffer_send.empty()) {
        auto &front = _buffer_send.front();
        uint32_t offset = 0;
        while (offset < front->size()) {
            auto nwrite = SSL_write(_ssl.get(), front->data() + offset, front->size() - offset);
            if (nwrite > 0) {
                //部分或全部写入完毕
                offset += nwrite;
                flushWriteBio();
                continue;
            }
            //nwrite <= 0,出现异常
            break;
        }

        if (offset != front->size()) {
            //这个包未消费完毕，出现了异常,清空数据并断开ssl
            ErrorL << "Ssl error on SSL_write: " << SSLUtil::getLastError();
            shutdown();
            break;
        }

        //这个包消费完毕，开始消费下一个包
        _buffer_send.pop_front();
    }
#endif //defined(ENABLE_OPENSSL)
}

bool SSL_Box::setHost(const char *host) {
    if (!_ssl) {
        return false;
    }
#ifdef SSL_ENABLE_SNI
    return 0 != SSL_set_tlsext_host_name(_ssl.get(), host);
#else
    return false;
#endif//SSL_ENABLE_SNI
}

} /* namespace turbo */


