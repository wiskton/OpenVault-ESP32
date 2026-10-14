#pragma once
#include <string>
namespace openvault {
constexpr unsigned walletSlots=5;
inline std::string walletStorageKey(const std::string& base,unsigned slot){return slot==0?base:base+std::to_string(slot);}
inline int parseSlot(const std::string& command){return command.size()==8&&command.compare(0,7,"SELECT ")==0&&command[7]>='0'&&command[7]<char('0'+walletSlots)?command[7]-'0':-1;}
inline bool validWalletName(const std::string& name){
 if(name.empty()||name.size()>18||name.front()==' '||name.back()==' ')return false;
 for(unsigned char c:name)if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c==' '||c=='-'||c=='_'))return false;
 return true;
}
struct CreateRequest {bool valid=false;bool mainnet=false;unsigned words=12;};
inline CreateRequest parseCreate(const std::string& command){
 CreateRequest out;
 for(const auto& network:{std::string("TEST"),std::string("BTC")})for(unsigned count:{12u,24u})
  if(command=="CREATE "+network+" "+std::to_string(count)){out.valid=true;out.mainnet=network=="BTC";out.words=count;}
 return out;
}
inline bool allowDelete(unsigned requested,unsigned active,bool saved,bool unlocked,bool idle){return requested<walletSlots&&requested==active&&saved&&unlocked&&idle;}
inline bool allowCreate(bool saved,bool unlocked,bool idle){return !saved&&!unlocked&&idle;}
}
