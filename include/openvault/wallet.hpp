#pragma once
#include <array>
#include <string>
#include <vector>
#include <cstdint>
extern "C" {
#include <wally_core.h>
#include <wally_bip39.h>
#include <wally_bip32.h>
#include <wally_crypto.h>
#include <wally_address.h>
#include <wally_psbt.h>
#include <wally_map.h>
#include <wally_transaction.h>
}
namespace openvault {
inline void wipe(void* data, size_t len) { volatile uint8_t* p=static_cast<volatile uint8_t*>(data); while(len--) *p++=0; }
inline void clearSecret(std::string& s) { if(!s.empty()) wipe(&s[0],s.size()); s.clear(); }
struct Payment { std::string address; uint64_t sats=0; bool change=false; };
class Wallet {
public:
    Wallet()=default;
    ~Wallet(){lock();}
    Wallet(const Wallet&)=delete;
    Wallet& operator=(const Wallet&)=delete;
    bool setEntropy(const uint8_t* data,size_t len,bool mainnet=false);
    bool importWords(const std::string& words,bool mainnet=false);
    void lock();
    bool ready() const {return active_;}
    bool mainnet() const {return main_;}
    size_t wordCount() const {return entropy_len_==32?24:12;}
    std::string mnemonicWord(size_t index) const;
    std::string address(uint32_t index=0,uint32_t chain=0) const;
    std::string accountXpub() const;
    std::string fingerprint() const;
    static bool validPin(const std::string& pin);
    bool seal(const std::string& pin,const std::array<uint8_t,32>& randomness,std::vector<uint8_t>& out) const;
    bool unlock(const std::vector<uint8_t>& blob,const std::string& pin);
    bool review(const std::string& base64,std::vector<Payment>& outputs,uint64_t& fee,std::string& error);
    bool signReviewed(std::string& signedBase64);
    uint32_t reviewedLocktime() const {return pending_&&pending_->tx?pending_->tx->locktime:0;}
    uint32_t reviewedVersion() const {return pending_&&pending_->tx?pending_->tx->version:0;}
    void cancelReview();
    static bool selfTest();
private:
    bool derive(uint32_t index,uint32_t chain,ext_key& key) const;
    bool matchPath(const wally_map_item& item,ext_key& derived,bool changeOnly) const;
    bool active_=false,main_=false;
    size_t entropy_len_=0;
    std::array<uint8_t,32> entropy_{};
    ext_key root_{};
    wally_psbt* pending_=nullptr;
};
}
