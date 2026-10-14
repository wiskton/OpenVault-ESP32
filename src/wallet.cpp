#include "openvault/wallet.hpp"
#include <algorithm>
#include <cstring>
namespace openvault {
namespace {
constexpr uint32_t hardened=0x80000000U;
constexpr uint64_t maxMoney=2100000000000000ULL;
constexpr uint32_t rounds=100000;
uint32_t le32(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
void put32(uint8_t* p,uint32_t n){for(int i=0;i<4;++i)p[i]=uint8_t(n>>(8*i));}
bool equal(const uint8_t* a,const uint8_t* b,size_t n){uint8_t diff=0;while(n--)diff|=*a++ ^ *b++;return diff==0;}
std::string takeString(char* p){if(!p)return {};std::string out(p);wally_free_string(p);return out;}
bool scriptMatch(const ext_key& key,const uint8_t* script,size_t len){return script&&len==22&&script[0]==0&&script[1]==20&&equal(script+2,key.hash160,20);}
}
void Wallet::cancelReview(){if(pending_) wally_psbt_free(pending_);pending_=nullptr;}
void Wallet::lock(){cancelReview();wipe(entropy_.data(),entropy_.size());wipe(&root_,sizeof(root_));active_=false;entropy_len_=0;main_=false;}
bool Wallet::setEntropy(const uint8_t* data,size_t len,bool mainnet){
    lock();if(!data||(len!=16&&len!=32))return false;
    char* words=nullptr;uint8_t seed[64]{};size_t written=0;
    if(bip39_mnemonic_from_bytes(nullptr,data,len,&words)!=WALLY_OK)return false;
    int ret=bip39_mnemonic_to_seed(words,"",seed,sizeof(seed),&written);
    wally_free_string(words);
    if(ret==WALLY_OK&&written!=64) ret=WALLY_ERROR;
    if(ret==WALLY_OK) ret=bip32_key_from_seed(seed,64,mainnet?BIP32_VER_MAIN_PRIVATE:BIP32_VER_TEST_PRIVATE,0,&root_);
    wipe(seed,sizeof(seed));
    if(ret!=WALLY_OK){lock();return false;}
    std::copy(data,data+len,entropy_.begin());entropy_len_=len;main_=mainnet;active_=true;return true;
}
bool Wallet::importWords(const std::string& words,bool mainnet){
    uint8_t entropy[32]{};size_t len=0;
    bool ok=bip39_mnemonic_to_bytes(nullptr,words.c_str(),entropy,sizeof(entropy),&len)==WALLY_OK;
    if(ok)ok=setEntropy(entropy,len,mainnet);
    wipe(entropy,sizeof(entropy));return ok;
}
std::string Wallet::mnemonicWord(size_t index) const {
    if(!active_||index>=wordCount())return {};
    char* phrase=nullptr;if(bip39_mnemonic_from_bytes(nullptr,entropy_.data(),entropy_len_,&phrase)!=WALLY_OK)return {};
    const char* p=phrase;for(size_t i=0;i<index;++i){p=std::strchr(p,' ');if(!p){wally_free_string(phrase);return {};}++p;}
    const char* end=std::strchr(p,' ');std::string out(p,end?size_t(end-p):std::strlen(p));wally_free_string(phrase);return out;
}
bool Wallet::derive(uint32_t index,uint32_t chain,ext_key& key)const{
    if(!active_||index>=100000||chain>1)return false;
    uint32_t path[]={84|hardened,(main_?0U:1U)|hardened,hardened,chain,index};
    bool ok=bip32_key_from_parent_path(&root_,path,5,0,&key)==WALLY_OK;
    if(!ok)wipe(&key,sizeof(key));
    return ok;
}
std::string Wallet::address(uint32_t index,uint32_t chain) const {
    ext_key key{};if(!derive(index,chain,key))return {};
    uint8_t script[22]={0,20};std::copy(key.hash160,key.hash160+20,script+2);wipe(&key,sizeof(key));
    char* addr=nullptr;if(wally_addr_segwit_from_bytes(script,22,main_?"bc":"tb",0,&addr)!=WALLY_OK)return {};return takeString(addr);
}
std::string Wallet::accountXpub() const {
    if(!active_)return {};
    uint32_t path[]={84|hardened,(main_?0U:1U)|hardened,hardened};ext_key key{};char* text=nullptr;
    if(bip32_key_from_parent_path(&root_,path,3,0,&key)!=WALLY_OK){wipe(&key,sizeof(key));return {};}
    const int result=bip32_key_to_base58(&key,BIP32_FLAG_KEY_PUBLIC,&text);wipe(&key,sizeof(key));return result==WALLY_OK?takeString(text):std::string();
}
std::string Wallet::fingerprint()const{
    if(!active_)return {};
    static const char* hex="0123456789abcdef";std::string out;
    for(size_t i=0;i<4;++i){out+=hex[root_.hash160[i]>>4];out+=hex[root_.hash160[i]&15];}return out;
}
bool Wallet::validPin(const std::string& pin){return pin.size()==8&&std::all_of(pin.begin(),pin.end(),[](char c){return c>='0'&&c<='9';});}
bool Wallet::seal(const std::string& pin,const std::array<uint8_t,32>& randomness,std::vector<uint8_t>& out) const {
    out.clear();if(!active_||!validPin(pin))return false;
    uint8_t keys[64]{},plain[34]{};plain[0]=main_?1:0;plain[1]=uint8_t(entropy_len_);std::copy(entropy_.begin(),entropy_.end(),plain+2);
    out.resize(120);std::memcpy(out.data(),"OVW1",4);put32(out.data()+4,rounds);std::copy(randomness.begin(),randomness.end(),out.begin()+8);
    size_t written=0;
    int ret=wally_pbkdf2_hmac_sha512(reinterpret_cast<const uint8_t*>(pin.data()),pin.size(),out.data()+8,16,0,rounds,keys,64);
    if(ret==WALLY_OK)ret=wally_aes_cbc(keys,32,out.data()+24,16,plain,sizeof(plain),AES_FLAG_ENCRYPT,out.data()+40,48,&written);
    if(ret==WALLY_OK&&written==48)ret=wally_hmac_sha256(keys+32,32,out.data(),88,out.data()+88,32);else ret=WALLY_ERROR;
    wipe(keys,sizeof(keys));wipe(plain,sizeof(plain));if(ret!=WALLY_OK){wipe(out.data(),out.size());out.clear();}return ret==WALLY_OK;
}
bool Wallet::unlock(const std::vector<uint8_t>& blob,const std::string& pin){
    lock();if(blob.size()!=120||std::memcmp(blob.data(),"OVW1",4)||le32(blob.data()+4)!=rounds||!validPin(pin))return false;
    uint8_t keys[64]{},tag[32]{},plain[48]{};size_t written=0;
    int ret=wally_pbkdf2_hmac_sha512(reinterpret_cast<const uint8_t*>(pin.data()),pin.size(),blob.data()+8,16,0,rounds,keys,64);
    if(ret==WALLY_OK)ret=wally_hmac_sha256(keys+32,32,blob.data(),88,tag,32);
    if(ret==WALLY_OK&&!equal(tag,blob.data()+88,32))ret=WALLY_ERROR;
    if(ret==WALLY_OK)ret=wally_aes_cbc(keys,32,blob.data()+24,16,blob.data()+40,48,AES_FLAG_DECRYPT,plain,sizeof(plain),&written);
    bool ok=ret==WALLY_OK&&written==34&&plain[0]<=1&&(plain[1]==16||plain[1]==32);
    if(ok)ok=setEntropy(plain+2,plain[1],plain[0]==1);
    wipe(keys,sizeof(keys));wipe(tag,sizeof(tag));wipe(plain,sizeof(plain));return ok;
}
bool Wallet::matchPath(const wally_map_item& item,ext_key& derived,bool changeOnly)const {
    if(item.key_len!=33||item.value_len!=24||!equal(item.value,root_.hash160,4))return false;
    uint32_t path[5];for(size_t i=0;i<5;++i)path[i]=le32(item.value+4+i*4);
    if(path[0]!=(84|hardened)||path[1]!=((main_?0U:1U)|hardened)||path[2]!=hardened||path[3]>1||path[4]>=100000||(changeOnly&&path[3]!=1))return false;
    return bip32_key_from_parent_path(&root_,path,5,0,&derived)==WALLY_OK&&equal(item.key,derived.pub_key,33);
}
bool Wallet::review(const std::string& base64,std::vector<Payment>& outputs,uint64_t& fee,std::string& error){
    cancelReview();outputs.clear();fee=0;error.clear();
    auto fail=[&](const char* text){error=text;outputs.clear();cancelReview();return false;};
    if(!active_)return fail("Carteira bloqueada");
    if(base64.empty()||base64.size()>24000)return fail("PSBT muito grande");
    if(wally_psbt_from_base64(base64.c_str(),WALLY_PSBT_PARSE_FLAG_STRICT|WALLY_PSBT_PARSE_FLAG_COMPLETE,&pending_)!=WALLY_OK)return fail("PSBT invalida");
    auto* p=pending_;
    if(p->version!=0||!p->tx||p->num_inputs<1||p->num_inputs>8||p->num_outputs<1||p->num_outputs>8||p->num_inputs!=p->tx->num_inputs||p->num_outputs!=p->tx->num_outputs)return fail("Politica: PSBT v0");
    uint64_t totalIn=0,totalOut=0;
    for(size_t i=0;i<p->num_inputs;++i){
        const auto& in=p->inputs[i];const auto& txin=p->tx->inputs[i];
        // Require the full previous transaction even for segwit, to authenticate amounts.
        if(!in.utxo||txin.index>=in.utxo->num_outputs||in.sighash>1||in.final_witness||in.signatures.num_items||in.keypaths.num_items!=1)return fail("UTXO/path ausente");
        uint8_t txid[32];if(wally_tx_get_txid(in.utxo,txid,32)!=WALLY_OK||!equal(txid,txin.txhash,32))return fail("UTXO nao confere");
        const auto& prev=in.utxo->outputs[txin.index];ext_key child{};
        bool mine=matchPath(in.keypaths.items[0],child,false)&&scriptMatch(child,prev.script,prev.script_len);wipe(&child,sizeof(child));
        if(!mine)return fail("Entrada nao e sua");
        if(in.witness_utxo&&(in.witness_utxo->satoshi!=prev.satoshi||in.witness_utxo->script_len!=prev.script_len||!equal(in.witness_utxo->script,prev.script,prev.script_len)))return fail("Witness divergente");
        if(!in.witness_utxo && wally_psbt_input_set_witness_utxo_from_tx(&p->inputs[i],in.utxo,txin.index)!=WALLY_OK) return fail("Witness indisponivel");
        if(prev.satoshi>maxMoney||totalIn>maxMoney-prev.satoshi)return fail("Valor invalido");
        totalIn+=prev.satoshi;
    }
    for(size_t i=0;i<p->num_outputs;++i){
        const auto& txout=p->tx->outputs[i];char* addr=nullptr;
        int rc=(txout.script_len>=4&&txout.script[0]==0)?wally_addr_segwit_from_bytes(txout.script,txout.script_len,main_?"bc":"tb",0,&addr):wally_scriptpubkey_to_address(txout.script,txout.script_len,main_?WALLY_NETWORK_BITCOIN_MAINNET:WALLY_NETWORK_BITCOIN_TESTNET,&addr);
        if(rc!=WALLY_OK)return fail("Saida nao suportada");
        Payment payment;payment.address=takeString(addr);payment.sats=txout.satoshi;
        const auto& paths=p->outputs[i].keypaths;
        if(paths.num_items==1){ext_key key{};payment.change=matchPath(paths.items[0],key,true)&&scriptMatch(key,txout.script,txout.script_len);wipe(&key,sizeof(key));}
        if(txout.satoshi>maxMoney||totalOut>maxMoney-txout.satoshi)return fail("Valor invalido");
        totalOut+=txout.satoshi;outputs.push_back(payment);
    }
    if(totalOut>totalIn)return fail("Taxa invalida");
    fee=totalIn-totalOut;
    if(fee>1000000ULL||fee>totalIn/10)return fail("Taxa acima limite");
    return true;
}
bool Wallet::signReviewed(std::string& signedBase64){
    signedBase64.clear();if(!active_||!pending_)return false;
    int rc=wally_psbt_sign_bip32(pending_,&root_,EC_FLAG_GRIND_R);
    if(rc==WALLY_OK)for(size_t i=0;i<pending_->num_inputs;++i)if(pending_->inputs[i].signatures.num_items!=1)rc=WALLY_ERROR;
    char* text=nullptr;if(rc==WALLY_OK)rc=wally_psbt_to_base64(pending_,0,&text);
    if(rc==WALLY_OK)signedBase64=takeString(text);
    cancelReview();return rc==WALLY_OK;
}
bool Wallet::selfTest(){
    uint8_t zero[16]{};Wallet test;if(!test.setEntropy(zero,16,true))return false;
    bool ok=test.mnemonicWord(0)=="abandon"&&test.mnemonicWord(11)=="about"&&test.address()=="bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu"&&test.fingerprint()=="73c5da0a";
    test.lock();return ok;
}
}
